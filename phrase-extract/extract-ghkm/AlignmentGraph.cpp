/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "AlignmentGraph.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <stack>

#include "SyntaxTree.h"

#include "ComposedRule.h"
#include "Node.h"
#include "Options.h"
#include "Subgraph.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

AlignmentGraph::AlignmentGraph(const SyntaxTree *t,
                               const std::vector<std::string> &s,
                               const Alignment &a)
{
  // Copy the parse tree nodes and add them to m_targetNodes.
  m_root = CopyParseTree(t);

  // Create a node for each source word.
  m_sourceNodes.reserve(s.size());
  for (std::vector<std::string>::const_iterator p(s.begin());
       p != s.end(); ++p) {
    m_sourceNodes.push_back(new Node(*p, SOURCE));
  }

  // Connect source nodes to parse tree leaves according to the given word
  // alignment.
  std::vector<Node *> targetTreeLeaves;
  GetTargetTreeLeaves(m_root, targetTreeLeaves);
  for (Alignment::const_iterator p(a.begin()); p != a.end(); ++p) {
    Node *src = m_sourceNodes[p->first];
    Node *tgt = targetTreeLeaves[p->second];
    src->AddParent(tgt);
    tgt->AddChild(src);
  }

  // Attach unaligned source words (if any).
  AttachUnalignedSourceWords();

  // Populate node spans.
  std::vector<Node *>::const_iterator p(m_sourceNodes.begin());
  for (int i = 0; p != m_sourceNodes.end(); ++p, ++i) {
    (*p)->PropagateIndex(i);
  }

  // Calculate complement spans.
  CalcComplementSpans(m_root);
}

AlignmentGraph::~AlignmentGraph()
{
  for (std::vector<Node *>::iterator p(m_sourceNodes.begin());
       p != m_sourceNodes.end(); ++p) {
    delete *p;
  }
  for (std::vector<Node *>::iterator p(m_targetNodes.begin());
       p != m_targetNodes.end(); ++p) {
    delete *p;
  }
}

Subgraph AlignmentGraph::ComputeMinimalFrontierGraphFragment(
  Node *root,
  const std::set<Node *> &frontierSet)
{
  std::stack<Node *> expandableNodes;
  std::set<const Node *> expandedNodes;

  if (root->IsSink()) {
    expandedNodes.insert(root);
  } else {
    expandableNodes.push(root);
  }

  while (!expandableNodes.empty()) {
    Node *n = expandableNodes.top();
    expandableNodes.pop();

    const std::vector<Node *> &children = n->GetChildren();

    for (std::vector<Node *>::const_iterator p(children.begin());
         p != children.end(); ++p) {
      Node *child = *p;
      if (child->IsSink()) {
        expandedNodes.insert(child);
        continue;
      }
      std::set<Node *>::const_iterator q = frontierSet.find(child);
      if (q == frontierSet.end()) { //child is not from the frontier set
        expandableNodes.push(child);
      } else if (child->GetType() == TARGET) { // still need source word
        expandableNodes.push(child);
      } else {
        expandedNodes.insert(child);
      }
    }
  }

  return Subgraph(root, expandedNodes);
}

void AlignmentGraph::ExtractMinimalRules(const Options &options)
{
  // Determine which nodes are frontier nodes.
  std::set<Node *> frontierSet;
  ComputeFrontierSet(m_root, options, frontierSet);

  // Form the minimal frontier graph fragment rooted at each frontier node.
  std::vector<Subgraph> fragments;
  fragments.reserve(frontierSet.size());
  for (std::set<Node *>::iterator p(frontierSet.begin());
       p != frontierSet.end(); ++p) {
    Node *root = *p;
    Subgraph fragment = ComputeMinimalFrontierGraphFragment(root, frontierSet);
    assert(!fragment.IsTrivial());
    // Can it form an SCFG rule?
    // FIXME Does this exclude non-lexical unary rules?
    if (root->GetType() == TREE && !root->GetSpan().empty()) {
      root->AddRule(new Subgraph(fragment));
    }
  }
}

void AlignmentGraph::ExtractComposedRules(const Options &options)
{
  ExtractComposedRules(m_root, options);
}

void AlignmentGraph::ExtractComposedRules(Node *node, const Options &options)
{
  // Extract composed rules for all children first.
  const std::vector<Node *> &children = node->GetChildren();
  for (std::vector<Node *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    ExtractComposedRules(*p, options);
  }

  // If there is no minimal rule for this node then there are no composed
  // rules.
  const std::vector<const Subgraph*> &rules = node->GetRules();
  assert(rules.size() <= 1);
  if (rules.empty()) {
    return;
  }

  // Construct an initial composition candidate from the minimal rule.
  ComposedRule cr(*(rules[0]));
  if (!cr.GetOpenAttachmentPoint()) {
    // No composition possible.
    return;
  }

  std::queue<ComposedRule> queue;
  queue.push(cr);
  while (!queue.empty()) {
    ComposedRule cr = queue.front();
    queue.pop();
    const Node *attachmentPoint = cr.GetOpenAttachmentPoint();
    assert(attachmentPoint);
    assert(attachmentPoint != node);
    // Create all possible rules by composing this node's minimal rule with the
    // existing rules (both minimal and composed) rooted at the first open
    // attachment point.
    const std::vector<const Subgraph*> &rules = attachmentPoint->GetRules();
    for (std::vector<const Subgraph*>::const_iterator p = rules.begin();
         p != rules.end(); ++p) {
      assert((*p)->GetRoot()->GetType() == TREE);
      ComposedRule *cr2 = cr.AttemptComposition(**p, options);
      if (cr2) {
        node->AddRule(new Subgraph(cr2->CreateSubgraph()));
        if (cr2->GetOpenAttachmentPoint()) {
          queue.push(*cr2);
        }
        delete cr2;
      }
    }
    // Done with this attachment point.  Advance to the next, if any.
    cr.CloseAttachmentPoint();
    if (cr.GetOpenAttachmentPoint()) {
      queue.push(cr);
    }
  }
}

Node *AlignmentGraph::CopyParseTree(const SyntaxTree *root)
{
  NodeType nodeType = (root->IsLeaf()) ? TARGET : TREE;

  std::auto_ptr<Node> n(new Node(root->value().label, nodeType));

  if (nodeType == TREE) {
    float score = 0.0f;
    SyntaxNode::AttributeMap::const_iterator p =
      root->value().attributes.find("pcfg");
    if (p != root->value().attributes.end()) {
      score = std::atof(p->second.c_str());
    }
    n->SetPcfgScore(score);
  }

  const std::vector<SyntaxTree *> &children = root->children();
  std::vector<Node *> childNodes;
  childNodes.reserve(children.size());
  for (std::vector<SyntaxTree *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    Node *child = CopyParseTree(*p);
    child->AddParent(n.get());
    childNodes.push_back(child);
  }
  n->SetChildren(childNodes);

  Node *p = n.release();
  m_targetNodes.push_back(p);
  return p;
}

// Recursively constructs the set of frontier nodes for the tree (or subtree)
// rooted at the given node.
void AlignmentGraph::ComputeFrontierSet(Node *root,
                                        const Options &options,
                                        std::set<Node *> &frontierSet) const
{
  // Non-tree nodes and unaligned target subtrees are not frontier nodes (and
  // nor are their descendants).  See the comment for the function
  // AlignmentGraph::IsFrontierNode().
  if (root->GetType() != TREE || root->GetSpan().empty()) {
    return;
  }

  if (IsFrontierNode(*root, options)) {
    frontierSet.insert(root);
  }

  // Recursively check descendants.
  const std::vector<Node *> &children = root->GetChildren();
  for (std::vector<Node *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    ComputeFrontierSet(*p, options, frontierSet);
  }
}

// Determines whether the given node is a frontier node or not. The definition
// of a frontier node differs from Galley et al's (2004) in the following ways:
//
// 1. A node with an empty span is not a frontier node (this is to exclude
//    unaligned target subtrees).
// 2. Target word nodes are not frontier nodes.
// 3. Source word nodes are not frontier nodes.
// 4. Unless the --AllowUnary option is used, a node is not a frontier node if
//    it has the same span as its parent.
bool AlignmentGraph::IsFrontierNode(const Node &n, const Options &options) const
{
  // Don't include word nodes or unaligned target subtrees.
  if (n.GetType() != TREE || n.GetSpan().empty()) {
    return false;
  }
  // This is the original GHKM definition of a frontier node.
  if (SpansIntersect(n.GetComplementSpan(), Closure(n.GetSpan()))) {
    return false;
  }
  // Unless unary rules are explicitly allowed, we use Chung et al's (2011)
  // modified defintion of a frontier node to eliminate the production of
  // non-lexical unary rules.
  assert(n.GetParents().size() <= 1);
  if (!options.allowUnary &&
      !n.GetParents().empty() &&
      n.GetParents()[0]->GetSpan() == n.GetSpan()) {
    return false;
  }
  return true;
}

void AlignmentGraph::CalcComplementSpans(Node *root)
{
  Span compSpan;
  std::set<Node *> siblings;

  const std::vector<Node *> &parents = root->GetParents();
  for (std::vector<Node *>::const_iterator p(parents.begin());
       p != parents.end(); ++p) {
    const Span &parentCompSpan = (*p)->GetComplementSpan();
    compSpan.insert(parentCompSpan.begin(), parentCompSpan.end());
    const std::vector<Node *> &c = (*p)->GetChildren();
    siblings.insert(c.begin(), c.end());
  }

  for (std::set<Node *>::iterator p(siblings.begin());
       p != siblings.end(); ++p) {
    if (*p == root) {
      continue;
    }
    const Span &siblingSpan = (*p)->GetSpan();
    compSpan.insert(siblingSpan.begin(), siblingSpan.end());
  }

  root->SetComplementSpan(compSpan);

  const std::vector<Node *> &children = root->GetChildren();
  for (std::vector<Node *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    CalcComplementSpans(*p);
  }
}

void AlignmentGraph::GetTargetTreeLeaves(Node *root,
    std::vector<Node *> &leaves)
{
  if (root->IsSink()) {
    leaves.push_back(root);
  } else {
    const std::vector<Node *> &children = root->GetChildren();
    for (std::vector<Node *>::const_iterator p(children.begin());
         p != children.end(); ++p) {
      GetTargetTreeLeaves(*p, leaves);
    }
  }
}

void AlignmentGraph::AttachUnalignedSourceWords()
{
  // Find the unaligned source words (if any).
  std::set<int> unaligned;
  for (size_t i = 0; i < m_sourceNodes.size(); ++i) {
    const Node &sourceNode = (*m_sourceNodes[i]);
    if (sourceNode.GetParents().empty()) {
      unaligned.insert(i);
    }
  }

  // Determine the attachment point for each one and attach it.
  for (std::set<int>::iterator p = unaligned.begin();
       p != unaligned.end(); ++p) {
    int index = *p;
    Node *attachmentPoint = DetermineAttachmentPoint(index);
    Node *sourceNode = m_sourceNodes[index];
    attachmentPoint->AddChild(sourceNode);
    sourceNode->AddParent(attachmentPoint);
  }
}

Node *AlignmentGraph::DetermineAttachmentPoint(int index)
{
  // Find the nearest aligned neighbour to the left, if any.
  int i = index;
  while (--i >= 0) {
    if (!m_sourceNodes[i]->GetParents().empty()) {
      break;
    }
  }
  // No aligned neighbours to the left, so attach to the root.
  if (i == -1) {
    return m_root;
  }
  // Find the nearest aligned neighbour to the right, if any.
  size_t j = index;
  while (++j < m_sourceNodes.size()) {
    if (!m_sourceNodes[j]->GetParents().empty()) {
      break;
    }
  }
  // No aligned neighbours to the right, so attach to the root.
  if (j == m_sourceNodes.size()) {
    return m_root;
  }
  // Construct the set of target nodes that are aligned to the left and right
  // neighbours.
  const std::vector<Node *> &leftParents = m_sourceNodes[i]->GetParents();
  assert(!leftParents.empty());
  const std::vector<Node *> &rightParents = m_sourceNodes[j]->GetParents();
  assert(!rightParents.empty());
  std::set<Node *> targetSet;
  targetSet.insert(leftParents.begin(), leftParents.end());
  targetSet.insert(rightParents.begin(), rightParents.end());
  // The attachment point is the lowest common ancestor of the target word
  // nodes, unless the LCA is itself a target word, in which case the LCA
  // is the parent.  This is to avoid including introducing new word alignments.
  // It assumes that the parse tree uses preterminals for parts of speech.
  Node *lca = Node::LowestCommonAncestor(targetSet.begin(), targetSet.end());
  if (lca->GetType() == TARGET) {
    assert(lca->GetParents().size() == 1);
    return lca->GetParents()[0];
  }
  return lca;
}

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining

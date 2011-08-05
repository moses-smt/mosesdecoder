/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

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

#include "Rule.h"
#include "Subgraph.h"

#include <algorithm>
#include <cassert>
#include <memory>

namespace
{
Node *
copyParseTree(const ParseTree * root, std::vector<Node *> & nodes)
{
  NodeType nodeType = (root->isLeaf()) ? TARGET : TREE;

  std::auto_ptr<Node> n(new Node(root->getLabel(), nodeType));

  const std::vector<ParseTree *> & children = root->getChildren();
  std::vector<Node *> childNodes;
  childNodes.reserve(children.size());
  for (std::vector<ParseTree *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    Node * child = copyParseTree(*p, nodes);
    child->addParent(n.get());
    childNodes.push_back(child);
  }
  n->setChildren(childNodes);

  Node * n2 = n.release();
  nodes.push_back(n2);
  return n2;
}

void
computeFrontierSet(Node * root, std::set<Node *> & frontierSet)
{
  // TODO Return if frontierSet already contains entry for root
  // TODO Or maintain set of visited nodes?

  if (!spansIntersect(root->getComplementSpan(), closure(root->getSpan()))) {
    frontierSet.insert(root);
  }

  const std::vector<Node *> & children = root->getChildren();
  for (std::vector<Node *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    computeFrontierSet(*p, frontierSet);
  }
}

void
calcComplementSpans(Node * root)
{
  Span & compSpan = root->getComplementSpan();

  std::set<Node *> siblings;

  const std::vector<Node *> & parents = root->getParents();
  for (std::vector<Node *>::const_iterator p(parents.begin());
       p != parents.end(); ++p) {
    const Span & parentCompSpan = (*p)->getComplementSpan();
    compSpan.insert(parentCompSpan.begin(), parentCompSpan.end());
    const std::vector<Node *> & c = (*p)->getChildren();
    siblings.insert(c.begin(), c.end());
  }

  for (std::set<Node *>::iterator p(siblings.begin());
       p != siblings.end(); ++p) {
    if (*p == root) {
      continue;
    }
    const Span & siblingSpan = (*p)->getSpan();
    compSpan.insert(siblingSpan.begin(), siblingSpan.end());
  }

  const std::vector<Node *> & children = root->getChildren();
  for (std::vector<Node *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    calcComplementSpans(*p);
  }
}

void
getTargetTreeLeaves(Node * root, std::vector<Node *> & leaves)
{
  if (root->isSink()) {
    leaves.push_back(root);
  } else {
    const std::vector<Node *> & children = root->getChildren();
    for (std::vector<Node *>::const_iterator p(children.begin());
         p != children.end(); ++p) {
      getTargetTreeLeaves(*p, leaves);
    }
  }
}

bool
partitionOrderComp(const Node * a, const Node * b)
{
  const Span & aSpan = a->getSpan();
  const Span & bSpan = b->getSpan();

  assert(!aSpan.empty() && !bSpan.empty());

  return *(aSpan.begin()) < *(bSpan.begin());
}

Rule
fragmentToRule(const Subgraph & fragment)
{
  // Source RHS

  std::set<Node *> sinkNodes(fragment.getSinkNodes());

  std::vector<Node *> sourceRHSNodes;
  for (std::set<Node *>::const_iterator p(sinkNodes.begin());
       p != sinkNodes.end(); ++p) {
    const Node & sinkNode = **p;
    if (!sinkNode.getSpan().empty()) {
      sourceRHSNodes.push_back(*p);
    }
  }

  std::sort(sourceRHSNodes.begin(), sourceRHSNodes.end(),
            partitionOrderComp);

  // Build a mapping from target nodes to source-order indices, so that we
  // can construct the Alignment object later.
  std::map<const Node *, std::vector<int> > sourceOrder;

  std::vector<Symbol> sourceRHS;
  int srcIndex = 0;
  for (std::vector<Node *>::const_iterator p(sourceRHSNodes.begin());
       p != sourceRHSNodes.end(); ++p, ++srcIndex) {
    const Node & sinkNode = **p;
    if (sinkNode.getType() == TREE) {
      sourceRHS.push_back(Symbol("X", NonTerminal));
      sourceOrder[&sinkNode].push_back(srcIndex);
    } else {
      assert(sinkNode.getType() == SOURCE);
      sourceRHS.push_back(Symbol(sinkNode.getLabel(), Terminal));
      // Add all aligned target words to the sourceOrder map
      const std::vector<Node *> & parents(sinkNode.getParents());
      for (std::vector<Node *>::const_iterator q(parents.begin());
           q != parents.end(); ++q) {
        assert((*q)->getType() == TARGET);
        sourceOrder[*q].push_back(srcIndex);
      }
    }
  }

  // Target RHS + alignment

  std::vector<Symbol> targetRHS;
  Alignment alignment;

  std::vector<Node *> leafNodes(fragment.getLeafNodes());

  alignment.reserve(leafNodes.size());  // might be too much but that's OK
  targetRHS.reserve(leafNodes.size());

  for (std::vector<Node *>::const_iterator p(leafNodes.begin());
       p != leafNodes.end(); ++p) {
    const Node & leaf = **p;
    if (leaf.getSpan().empty()) {
      // The node doesn't cover any source words, so we can only add
      // terminals to the target RHS (not a non-terminal).
      std::vector<std::string> targetWords(leaf.getTargetWords());
      for (std::vector<std::string>::const_iterator q(targetWords.begin());
           q != targetWords.end(); ++q) {
        targetRHS.push_back(Symbol(*q, Terminal));
      }
    } else {
      SymbolType type = (leaf.getType() == TREE) ? NonTerminal : Terminal;
      targetRHS.push_back(Symbol(leaf.getLabel(), type));

      int tgtIndex = targetRHS.size()-1;
      std::map<const Node *, std::vector<int> >::iterator q(sourceOrder.find(&leaf));
      assert(q != sourceOrder.end());
      std::vector<int> & sourceNodes = q->second;
      for (std::vector<int>::iterator r(sourceNodes.begin());
           r != sourceNodes.end(); ++r) {
        int srcIndex = *r;
        alignment.push_back(std::make_pair(srcIndex, tgtIndex));
      }
    }
  }

  assert(!alignment.empty());

  // Source LHS
  Symbol sourceLHS("X", NonTerminal);

  // Target LHS
  Symbol targetLHS(fragment.getRoot()->getLabel(), NonTerminal);

  return Rule(sourceLHS, targetLHS, sourceRHS, targetRHS, alignment);
}
}

void
Node::setChildren(const std::vector<Node*> & children)
{
  m_children = children;
}

void
Node::setParents(const std::vector<Node*> & parents)
{
  m_parents = parents;
}

void
Node::addChild(Node * child)
{
  m_children.push_back(child);
}

void
Node::addParent(Node * parent)
{
  m_parents.push_back(parent);
}

bool
Node::isSink() const
{
  return m_children.empty();
}

void
Node::propagateIndex(int index)
{
  m_span.insert(index);
  for (std::vector<Node *>::const_iterator p(m_parents.begin());
       p != m_parents.end(); ++p) {
    (*p)->propagateIndex(index);
  }
}

std::vector<std::string>
Node::getTargetWords() const
{
  std::vector<std::string> targetWords;
  getTargetWords(targetWords);
  return targetWords;
}

void
Node::getTargetWords(std::vector<std::string> & targetWords) const
{
  if (m_type == TARGET) {
    targetWords.push_back(m_label);
  } else {
    for (std::vector<Node *>::const_iterator p(m_children.begin());
         p != m_children.end(); ++p) {
      (*p)->getTargetWords(targetWords);
    }
  }
}

AlignmentGraph::AlignmentGraph(const ParseTree * t,
                               const std::vector<std::string> & s,
                               const Alignment & a)
{
  m_root = copyParseTree(t, m_targetNodes);

  m_sourceNodes.reserve(s.size());
  for (std::vector<std::string>::const_iterator p(s.begin());
       p != s.end(); ++p) {
    m_sourceNodes.push_back(new Node(*p, SOURCE));
  }

  std::vector<Node *> targetTreeLeaves;
  getTargetTreeLeaves(m_root, targetTreeLeaves);

  for (Alignment::const_iterator p(a.begin()); p != a.end(); ++p) {
    Node * src = m_sourceNodes[p->first];
    Node * tgt = targetTreeLeaves[p->second];
    src->addParent(tgt);
    tgt->addChild(src);
  }
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

std::vector<Rule>
AlignmentGraph::inferRules() const
{
  size_t i = 0;
  std::vector<Node *>::const_iterator p(m_sourceNodes.begin());
  for (; p != m_sourceNodes.end(); ++p, ++i) {
    (*p)->propagateIndex(i);
  }

  calcComplementSpans(m_root);

  std::set<Node *> frontierSet;
  computeFrontierSet(m_root, frontierSet);

  std::vector<Subgraph> fragments;
  for (std::set<Node *>::iterator p(frontierSet.begin());
       p != frontierSet.end(); ++p) {
    Subgraph subgraph(*p);
    while (!subgraph.expand(frontierSet)) {
      ;
    }
    if (subgraph.canFormSCFGRule()) {
      fragments.push_back(subgraph);
    }
  }

  std::vector<Rule> rules;
  for (std::vector<Subgraph>::const_iterator p = fragments.begin();
       p != fragments.end(); ++p) {
    rules.push_back(fragmentToRule(*p));
  }

  return rules;
}

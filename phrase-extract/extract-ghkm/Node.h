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

#pragma once
#ifndef EXTRACT_GHKM_NODE_H_
#define EXTRACT_GHKM_NODE_H_

#include <cassert>
#include <iterator>
#include <string>
#include <vector>

#include "Span.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

class Subgraph;

enum NodeType { SOURCE, TARGET, TREE };

class Node
{
public:
  Node(const std::string &label, NodeType type)
    : m_label(label)
    , m_type(type)
    , m_pcfgScore(0.0f) {}

  ~Node();

  const std::string &GetLabel() const {
    return m_label;
  }
  NodeType GetType() const {
    return m_type;
  }
  const std::vector<Node*> &GetChildren() const {
    return m_children;
  }
  const std::vector<Node*> &GetParents() const {
    return m_parents;
  }
  float GetPcfgScore() const {
    return m_pcfgScore;
  }
  const Span &GetSpan() const {
    return m_span;
  }
  const Span &GetComplementSpan() const {
    return m_complementSpan;
  }
  const std::vector<const Subgraph*> &GetRules() const {
    return m_rules;
  }

  void SetChildren(const std::vector<Node*> &c) {
    m_children = c;
  }
  void SetParents(const std::vector<Node*> &p) {
    m_parents = p;
  }
  void SetPcfgScore(float s) {
    m_pcfgScore = s;
  }
  void SetSpan(const Span &s) {
    m_span = s;
  }
  void SetComplementSpan(const Span &cs) {
    m_complementSpan = cs;
  }

  void AddChild(Node *c) {
    m_children.push_back(c);
  }
  void AddParent(Node *p) {
    m_parents.push_back(p);
  }
  void AddRule(const Subgraph *s) {
    m_rules.push_back(s);
  }

  bool IsSink() const {
    return m_children.empty();
  }
  bool IsPreterminal() const;

  void PropagateIndex(int);

  std::vector<std::string> GetTargetWords() const;

  // Gets the path from this node's parent to the root.  This node is
  // required to be part of the original parse tree (i.e. not a source word,
  // which can have multiple parents).
  template<typename OutputIterator>
  void GetTreeAncestors(OutputIterator result, bool includeSelf=false);

  // Returns the lowest common ancestor given a sequence of nodes belonging to
  // the target tree.
  template<typename InputIterator>
  static Node *LowestCommonAncestor(InputIterator first, InputIterator last);

private:
  // Disallow copying
  Node(const Node &);
  Node &operator=(const Node &);

  void GetTargetWords(std::vector<std::string> &) const;

  std::string m_label;
  NodeType m_type;
  std::vector<Node*> m_children;
  std::vector<Node*> m_parents;
  float m_pcfgScore;
  Span m_span;
  Span m_complementSpan;
  std::vector<const Subgraph*> m_rules;
};

template<typename OutputIterator>
void Node::GetTreeAncestors(OutputIterator result, bool includeSelf)
{
  // This function assumes the node is part of the parse tree.
  assert(m_type == TARGET || m_type == TREE);

  if (includeSelf) {
    *result++ = this;
  }

  Node *ancestor = !(m_parents.empty()) ? m_parents[0] : 0;
  while (ancestor != 0) {
    *result++ = ancestor;
    ancestor = !(ancestor->m_parents.empty()) ? ancestor->m_parents[0] : 0;
  }
}

template<typename InputIterator>
Node *Node::LowestCommonAncestor(InputIterator first, InputIterator last)
{
  // Check for an empty sequence.
  if (first == last) {
    return 0;
  }

  // Check for the case that the sequence contains only one distinct node.
  // Also check that every node belongs to the target tree.
  InputIterator p = first;
  Node *lca = *p++;
  for (; p != last; ++p) {
    Node *node = *p;
    assert(node->m_type != SOURCE);
    if (node != lca) {
      lca = 0;
    }
  }
  if (lca) {
    return lca;
  }

  // Now construct an ancestor path for each node, from itself to the root.
  size_t minPathLength = 0;
  std::vector<std::vector<Node *> > paths;
  for (p = first; p != last; ++p) {
    paths.resize(paths.size()+1);
    (*p)->GetTreeAncestors(std::back_inserter(paths.back()), true);
    size_t pathLength = paths.back().size();
    assert(pathLength > 0);
    if (paths.size() == 1 || pathLength < minPathLength) {
      minPathLength = pathLength;
    }
  }

  // Search for the start of the longest common suffix by working forward from
  // the the earliest possible starting point to the root.
  for (size_t i = 0; i < minPathLength; ++i) {
    bool match = true;
    for (size_t j = 0; j < paths.size(); ++j) {
      size_t index = paths[j].size() - minPathLength + i;
      assert(index >= 0);
      assert(index < paths[j].size());
      if (j == 0) {
        lca = paths[j][index];
        assert(lca);
      } else if (lca != paths[j][index]) {
        match = false;
        break;
      }
    }
    if (match) {
      return lca;
    }
  }

  // A lowest common ancestor should have been found.
  assert(false);
  return 0;
}

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining

#endif

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
#ifndef EXTRACT_GHKM_SUBGRAPH_H_
#define EXTRACT_GHKM_SUBGRAPH_H_

#include "Node.h"

#include <set>
#include <vector>

namespace Moses
{
namespace GHKM
{

class Node;

class Subgraph
{
public:
  Subgraph(const Node *root)
    : m_root(root)
    , m_depth(0)
    , m_size(root->GetType() == TREE ? 1 : 0)
    , m_nodeCount(1)
    , m_pcfgScore(0.0f) {}

  Subgraph(const Node *root, const std::set<const Node *> &leaves)
    : m_root(root)
    , m_leaves(leaves)
    , m_depth(-1)
    , m_size(-1)
    , m_nodeCount(-1)
    , m_pcfgScore(0.0f) {
    m_depth = CalcDepth(m_root);
    m_size = CalcSize(m_root);
    m_nodeCount = CountNodes(m_root);
    m_pcfgScore = CalcPcfgScore();
  }

  Subgraph(const Subgraph &other, bool targetOnly=false)
    : m_root(other.m_root)
    , m_leaves(other.m_leaves)
    , m_depth(other.m_depth)
    , m_size(other.m_size)
    , m_nodeCount(other.m_nodeCount)
    , m_pcfgScore(other.m_pcfgScore) {
    if (targetOnly && m_root->GetType() != SOURCE) {
      // Replace any source-word sink nodes with their parents (except for
      // the special case where the parent is a non-word tree node -- see
      // below).
      std::set<const Node *> targetLeaves;
      for (std::set<const Node *>::const_iterator p = m_leaves.begin();
           p != m_leaves.end(); ++p) {
        const Node *leaf = *p;
        if (leaf->GetType() != SOURCE) {
          targetLeaves.insert(leaf);
        } else {
          const std::vector<Node*> &parents = leaf->GetParents();
          for (std::vector<Node*>::const_iterator q = parents.begin();
               q != parents.end(); ++q) {
            const Node *parent = *q;
            // Only add parents that are words, not tree nodes since those
            // are never sink nodes.  (A source word can have a tree node as
            // its parent due to the heuristic for handling unaligned source
            // words).
            if (parent->GetType() == TARGET) {
              targetLeaves.insert(*q);
            }
          }
        }
      }
      m_leaves.swap(targetLeaves);
    }
  }

  const Node *GetRoot() const {
    return m_root;
  }
  const std::set<const Node *> &GetLeaves() const {
    return m_leaves;
  }
  int GetDepth() const {
    return m_depth;
  }
  int GetSize() const {
    return m_size;
  }
  int GetNodeCount() const {
    return m_nodeCount;
  }
  float GetPcfgScore() const {
    return m_pcfgScore;
  }

  bool IsTrivial() const {
    return m_leaves.empty();
  }

  void GetTargetLeaves(std::vector<const Node *> &) const;

  void PrintTree(std::ostream &out) const;

private:
  void GetTargetLeaves(const Node *, std::vector<const Node *> &) const;
  int CalcDepth(const Node *) const;
  int CalcSize(const Node *) const;
  float CalcPcfgScore() const;
  int CountNodes(const Node *) const;
  void RecursivelyPrintTree(const Node *n, std::ostream &out) const;

  const Node *m_root;
  std::set<const Node *> m_leaves;
  int m_depth;
  int m_size;
  int m_nodeCount;
  float m_pcfgScore;
};

}  // namespace GHKM
}  // namespace Moses

#endif

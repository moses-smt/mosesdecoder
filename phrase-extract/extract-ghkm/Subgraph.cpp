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

#include <iostream>
#include "Subgraph.h"
#include "Node.h"

namespace Moses
{
namespace GHKM
{

void Subgraph::GetTargetLeaves(std::vector<const Node *> &result) const
{
  result.clear();
  GetTargetLeaves(m_root, result);
}

void Subgraph::GetTargetLeaves(const Node *root,
                               std::vector<const Node *> &result) const
{
  if (root->GetType() == TARGET || m_leaves.find(root) != m_leaves.end()) {
    result.push_back(root);
  } else {
    const std::vector<Node*> &children = root->GetChildren();
    for (std::vector<Node *>::const_iterator p(children.begin());
         p != children.end(); ++p) {
      GetTargetLeaves(*p, result);
    }
  }
}

int Subgraph::CountNodes(const Node *n) const
{
  if (n->GetType() != TREE) {
    return 0;
  }
  if (IsTrivial()) {
    return 1;
  }
  int count = 1;
  const std::vector<Node*> &children = n->GetChildren();
  for (std::vector<Node *>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    const Node *child = *p;
    if (m_leaves.find(child) == m_leaves.end()) {
      count += CountNodes(child);
    } else if (child->GetType() == TREE) {
      ++count;
    }
  }
  return count;
}

int Subgraph::CalcSize(const Node *n) const
{
  if (n->GetType() != TREE || n->IsPreterminal()) {
    return 0;
  }
  if (IsTrivial()) {
    return 1;
  }
  int count = 1;
  const std::vector<Node*> &children = n->GetChildren();
  for (std::vector<Node *>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    if (m_leaves.find(*p) == m_leaves.end()) {
      count += CalcSize(*p);
    }
  }
  return count;
}

int Subgraph::CalcDepth(const Node *n) const
{
  if (n->GetType() != TREE || n->IsPreterminal() || m_leaves.empty()) {
    return 0;
  }
  int maxChildDepth = 0;
  const std::vector<Node*> &children = n->GetChildren();
  for (std::vector<Node *>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    if (m_leaves.find(*p) == m_leaves.end()) {
      maxChildDepth = std::max(maxChildDepth, CalcDepth(*p));
    }
  }
  return maxChildDepth + 1;
}

float Subgraph::CalcPcfgScore() const
{
  if (m_root->GetType() != TREE || m_leaves.empty()) {
    return 0.0f;
  }
  float score = m_root->GetPcfgScore();
  for (std::set<const Node *>::const_iterator p = m_leaves.begin();
       p != m_leaves.end(); ++p) {
    const Node *leaf = *p;
    if (leaf->GetType() == TREE) {
      score -= leaf->GetPcfgScore();
    }
  }
  return score;
}

void Subgraph::PrintTree(std::ostream &out) const 
{
  RecursivelyPrintTree(m_root,out);
}

void Subgraph::RecursivelyPrintTree(const Node *n, std::ostream &out) const 
{
  NodeType nodeType = n->GetType();
  if (nodeType == TREE) {
    out << "[" << n->GetLabel();
    if (m_leaves.find(n) == m_leaves.end()) {
      const std::vector<Node *> &children = n->GetChildren();
      for (std::vector<Node *>::const_iterator p(children.begin());
           p != children.end(); ++p) {
        Node *child = *p;
        out << " ";
        RecursivelyPrintTree(child,out);
      }
    }
    out << "]";
  } else if (nodeType == TARGET) {
    out << n->GetLabel();
  }
}

}  // namespace Moses
}  // namespace GHKM

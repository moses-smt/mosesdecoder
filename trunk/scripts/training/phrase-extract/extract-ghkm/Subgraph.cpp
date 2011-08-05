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

#include "Subgraph.h"

#include "AlignmentGraph.h"

#include <cassert>

Subgraph::Subgraph(Node * root)
  : m_root(root)
{
  if (root->isSink()) {
    m_expandedNodes.insert(root);
  } else {
    m_expandableNodes.push(root);
  }
}

bool
Subgraph::isFragment() const
{
  return !isTrivial();
}

bool
Subgraph::canFormSCFGRule() const
{
  return isFragment()
         && (m_root->getType() == TREE)
         && !(m_root->getSpan().empty());
}

bool
Subgraph::isTrivial() const
{
  std::set<Node *> sinkNodes = getSinkNodes();
  return (sinkNodes.size() == 1) &&
         (sinkNodes.find(m_root) != sinkNodes.end());
}

bool
Subgraph::isSinkNode(Node * n) const
{
  assert(m_expandableNodes.empty());
  return m_expandedNodes.find(n) != m_expandedNodes.end();
}

std::set<Node *>
Subgraph::getSinkNodes() const
{
  std::set<Node *> sinkNodes;
  std::stack<Node *> expandable(m_expandableNodes);
  while (!expandable.empty()) {
    sinkNodes.insert(expandable.top());
    expandable.pop();
  }
  sinkNodes.insert(m_expandedNodes.begin(), m_expandedNodes.end());
  return sinkNodes;
}

// Expand a single subgraph node.  Return true if subgraph is fully expanded
// or false otherwise.
bool
Subgraph::expand(const std::set<Node *> & frontierSet)
{
  if (m_expandableNodes.empty()) {
    return true;
  }

  Node * n = m_expandableNodes.top();
  m_expandableNodes.pop();

  const std::vector<Node *> & children = n->getChildren();
  for (std::vector<Node *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    Node * child = *p;
    if (child->isSink()) {
      m_expandedNodes.insert(child);
      continue;
    }
    std::set<Node *>::const_iterator q = frontierSet.find(child);
    if (q == frontierSet.end()) { //child is not from the frontier set
      m_expandableNodes.push(child);
    } else if (child->getType() == TARGET) { // still need source word
      m_expandableNodes.push(child);
    } else {
      m_expandedNodes.insert(child);
    }
  }

  return m_expandableNodes.empty();
}

std::vector<Node *>
Subgraph::getLeafNodes() const
{
  std::vector<Node *> leafNodes;
  std::set<Node *> sinkNodes(getSinkNodes());
  getLeafNodes(m_root, leafNodes, sinkNodes);
  return leafNodes;
}

void
Subgraph::getLeafNodes(Node * root, std::vector<Node *> & leafNodes,
                       const std::set<Node *> & sinkNodes) const
{
  if (root->getType() == TARGET || sinkNodes.find(root) != sinkNodes.end()) {
    leafNodes.push_back(root);
  } else {
    const std::vector<Node*> & children(root->getChildren());
    for (std::vector<Node *>::const_iterator p(children.begin());
         p != children.end(); ++p) {
      getLeafNodes(*p, leafNodes, sinkNodes);
    }
  }
}

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

#pragma once
#ifndef SUBGRAPH_H_INCLUDED_
#define SUBGRAPH_H_INCLUDED_

class Node;

#include <set>
#include <stack>
#include <vector>

class Subgraph
{
public:
  Subgraph(Node * root);

  bool
  isTrivial() const;

  bool
  isFragment() const;

  bool
  canFormSCFGRule() const;

  bool
  isSinkNode(Node *) const;

  bool
  expand(const std::set<Node *> & frontierSet);

  const Node *
  getRoot() const {
    return m_root;
  };

  std::set<Node *>
  getSinkNodes() const;

  std::vector<Node *>
  getLeafNodes() const;

private:
  Node * m_root;
  std::stack<Node *> m_expandableNodes;
  std::set<Node *> m_expandedNodes;

  void
  getLeafNodes(Node * root, std::vector<Node *> & leafNodes,
               const std::set<Node *> & sinkNodes) const;
};

#endif

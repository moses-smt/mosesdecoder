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
#ifndef ALIGNMENTGRAPH_H_INCLUDED_
#define ALIGNMENTGRAPH_H_INCLUDED_

#include "Alignment.h"
#include "ParseTree.h"
#include "Span.h"
#include "Rule.h"

#include <string>
#include <vector>

enum NodeType { SOURCE, TARGET, TREE };

class Node
{
public:

  Node(const std::string & label, NodeType type)
    : m_label(label)
    , m_type(type)
    , m_children()
    , m_parents()
  {}

  const std::string &
  getLabel() const {
    return m_label;
  }

  NodeType
  getType() const {
    return m_type;
  }

  const std::vector<Node*> &
  getChildren() const {
    return m_children;
  }

  const std::vector<Node*> &
  getParents() const {
    return m_parents;
  }

  void
  setChildren(const std::vector<Node*> &);

  void
  setParents(const std::vector<Node*> &);

  void
  addChild(Node *);

  void
  addParent(Node *);

  bool
  isSink() const;

  void
  propagateIndex(int);

  Span &
  getSpan() {
    return m_span;
  }

  const Span &
  getSpan() const {
    return m_span;
  }

  Span &
  getComplementSpan() {
    return m_complementSpan;
  }

  const Span &
  getComplementSpan() const {
    return m_complementSpan;
  }

  std::vector<std::string>
  getTargetWords() const;

private:
  std::string m_label;
  NodeType m_type;
  std::vector<Node*> m_children;
  std::vector<Node*> m_parents;
  Span m_span;
  Span m_complementSpan;

  // Disallow copying
  Node(const Node &);
  Node & operator=(const Node &);

  void
  getTargetWords(std::vector<std::string> &) const;
};

class AlignmentGraph
{
public:
  AlignmentGraph(const ParseTree *,
                 const std::vector<std::string> &,
                 const Alignment &);

  ~AlignmentGraph();

  Node *
  getRoot() {
    return m_root;
  }

  std::vector<Node *> &
  getSourceNodes() {
    return m_sourceNodes;
  }

  std::vector<Rule>
  inferRules() const;

private:
  Node * m_root;
  std::vector<Node *> m_sourceNodes;
  std::vector<Node *> m_targetNodes;

  // Disallow copying
  AlignmentGraph(const AlignmentGraph &);
  AlignmentGraph & operator=(const AlignmentGraph &);
};

#endif

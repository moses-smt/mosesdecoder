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
#ifndef PARSETREE_H_INCLUDED_
#define PARSETREE_H_INCLUDED_

#include <string>
#include <vector>

class ParseTree
{
public:
  ParseTree(const std::string & label)
    : m_label(label)
    , m_children()
    , m_parent()
  {}

  ~ParseTree();

  const std::string &
  getLabel() const {
    return m_label;
  }

  const std::vector<ParseTree*> &
  getChildren() const {
    return m_children;
  }

  const ParseTree *
  getParent() const {
    return m_parent;
  }

  void
  setParent(ParseTree *);

  void
  setChildren(const std::vector<ParseTree*> &);

  void
  addChild(ParseTree *);

  bool
  isLeaf() const;

private:
  std::string m_label;
  std::vector<ParseTree*> m_children;
  ParseTree * m_parent;

  // Disallow copying
  ParseTree(const ParseTree &);
  ParseTree & operator=(const ParseTree &);
};

#endif

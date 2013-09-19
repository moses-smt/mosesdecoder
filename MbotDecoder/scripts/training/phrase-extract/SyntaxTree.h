// $Id: SyntaxTree.h,v 1.1.1.1 2013/01/06 16:54:12 braunefe Exp $
// vim:tabstop=2

/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

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
#include <string>
#include <vector>
#include <map>
#include <sstream>

class SyntaxNode
{
protected:
  int m_start, m_end;
  std::string m_label;
  std::vector< SyntaxNode* > m_children;
  SyntaxNode* m_parent;
public:
  SyntaxNode( int startPos, int endPos, std::string label )
    :m_start(startPos)
    ,m_end(endPos)
    ,m_label(label)
  {}
  int GetStart() const {
    return m_start;
  }
  int GetEnd() const {
    return m_end;
  }
  std::string GetLabel() const {
    return m_label;
  }
};


typedef std::vector< int > SplitPoints;
typedef std::vector< SplitPoints > ParentNodes;

class SyntaxTree
{
protected:
  std::vector< SyntaxNode* > m_nodes;
  SyntaxNode* m_top;

  typedef std::map< int, std::vector< SyntaxNode* > > SyntaxTreeIndex2;
  typedef SyntaxTreeIndex2::const_iterator SyntaxTreeIndexIterator2;
  typedef std::map< int, SyntaxTreeIndex2 > SyntaxTreeIndex;
  typedef SyntaxTreeIndex::const_iterator SyntaxTreeIndexIterator;
  SyntaxTreeIndex m_index;
  std::vector< SyntaxNode* > m_emptyNode;

  friend std::ostream& operator<<(std::ostream&, const SyntaxTree&);

public:
  SyntaxTree() {
    m_emptyNode.clear();
  }
  ~SyntaxTree();

  void AddNode( int startPos, int endPos, std::string label );
  ParentNodes Parse();
  bool HasNode( int startPos, int endPos ) const;
  const std::vector< SyntaxNode* >& GetNodes( int startPos, int endPos ) const;
  const std::vector< SyntaxNode* >& GetAllNodes() {
    return m_nodes;
  };
  size_t GetNumWords() const {
    return m_index.size();
  }
  std::string ToString() const;
};

std::ostream& operator<<(std::ostream&, const SyntaxTree&);


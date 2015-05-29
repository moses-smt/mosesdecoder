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

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "SyntaxNode.h"

namespace MosesTraining
{

typedef std::vector< int > SplitPoints;
typedef std::vector< SplitPoints > ParentNodes;

class SyntaxNodeCollection
{
protected:
  std::vector< SyntaxNode* > m_nodes;
  SyntaxNode* m_top;

  typedef std::map< int, std::vector< SyntaxNode* > > SyntaxTreeIndex2;
  typedef SyntaxTreeIndex2::const_iterator SyntaxTreeIndexIterator2;
  typedef std::map< int, SyntaxTreeIndex2 > SyntaxTreeIndex;
  typedef SyntaxTreeIndex::const_iterator SyntaxTreeIndexIterator;
  SyntaxTreeIndex m_index;
  int m_size;
  std::vector< SyntaxNode* > m_emptyNode;

public:
  SyntaxNodeCollection()
    : m_top(0)  // m_top doesn't get set unless ConnectNodes is called.
    , m_size(0) {}

  ~SyntaxNodeCollection();

  SyntaxNode *AddNode( int startPos, int endPos, const std::string &label );

  SyntaxNode *GetTop() {
    return m_top;
  }

  ParentNodes Parse();
  bool HasNode( int startPos, int endPos ) const;
  const std::vector< SyntaxNode* >& GetNodes( int startPos, int endPos ) const;
  const std::vector< SyntaxNode* >& GetAllNodes() {
    return m_nodes;
  };
  size_t GetNumWords() const {
    return m_size;
  }
  void ConnectNodes();
  void Clear();
};

}  // namespace MosesTraining

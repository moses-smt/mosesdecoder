// $Id: SyntaxTree.h 1960 2008-12-15 12:52:38Z phkoehn $
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

namespace MosesTraining
{

class SyntaxNode
{
protected:
  int m_start, m_end;
  std::string m_label;
  std::vector< SyntaxNode* > m_children;
  SyntaxNode* m_parent;
  float m_pcfgScore;
public:
  SyntaxNode( int startPos, int endPos, std::string label )
    :m_start(startPos)
    ,m_end(endPos)
    ,m_label(label)
    ,m_parent(0)
    ,m_pcfgScore(0.0f) {
  }
  int GetStart() const {
    return m_start;
  }
  int GetEnd() const {
    return m_end;
  }
  std::string GetLabel() const {
    return m_label;
  }
  float GetPcfgScore() const {
    return m_pcfgScore;
  }
  void SetPcfgScore(float score) {
    m_pcfgScore = score;
  }
  SyntaxNode *GetParent() {
    return m_parent;
  }
  void SetParent(SyntaxNode *parent) {
    m_parent = parent;
  }
  void AddChild(SyntaxNode* child) {
    m_children.push_back(child);
  }
  const std::vector< SyntaxNode* > &GetChildren() const {
    return m_children;
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
  int m_size;
  std::vector< SyntaxNode* > m_emptyNode;

  friend std::ostream& operator<<(std::ostream&, const SyntaxTree&);

public:
  SyntaxTree() {
    m_top = 0;  // m_top doesn't get set unless ConnectNodes is called.
  }
  ~SyntaxTree();

  SyntaxNode *AddNode( int startPos, int endPos, std::string label );

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
  std::string ToString() const;
};

std::ostream& operator<<(std::ostream&, const SyntaxTree&);

}


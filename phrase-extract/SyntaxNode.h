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
  typedef std::map<std::string, std::string> AttributeMap;

  AttributeMap attributes;

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

}  // namespace MosesTraining

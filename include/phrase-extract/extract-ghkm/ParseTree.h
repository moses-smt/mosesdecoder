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
#ifndef EXTRACT_GHKM_PARSE_TREE_H_
#define EXTRACT_GHKM_PARSE_TREE_H_

#include <string>
#include <vector>

namespace Moses
{
namespace GHKM
{

class ParseTree
{
public:
  ParseTree(const std::string &label)
    : m_label(label)
    , m_parent(0)
    , m_pcfgScore(0.0) {}

  ~ParseTree();

  const std::string &GetLabel() const {
    return m_label;
  }
  const std::vector<ParseTree*> &GetChildren() const {
    return m_children;
  }
  const ParseTree *GetParent() const {
    return m_parent;
  }
  float GetPcfgScore() const {
    return m_pcfgScore;
  }

  void SetParent(ParseTree *);
  void SetChildren(const std::vector<ParseTree*> &);
  void SetPcfgScore(float score) {
    m_pcfgScore = score;
  }

  void AddChild(ParseTree *);

  bool IsLeaf() const;

  template<typename OutputIterator>
  void GetLeaves(OutputIterator);

private:
  // Disallow copying
  ParseTree(const ParseTree &);
  ParseTree &operator=(const ParseTree &);

  std::string m_label;
  std::vector<ParseTree*> m_children;
  ParseTree *m_parent;
  float m_pcfgScore;  // log probability
};

template<typename OutputIterator>
void ParseTree::GetLeaves(OutputIterator result)
{
  if (IsLeaf()) {
    *result++ = this;
  } else {
    std::vector<ParseTree *>::const_iterator p = m_children.begin();
    std::vector<ParseTree *>::const_iterator end = m_children.end();
    while (p != end) {
      ParseTree &child = **p++;
      child.GetLeaves(result);
    }
  }
}

}  // namespace GHKM
}  // namespace Moses

#endif

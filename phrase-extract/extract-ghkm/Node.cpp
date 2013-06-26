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

#include "Node.h"

#include "Subgraph.h"

namespace Moses
{
namespace GHKM
{

Node::~Node()
{
  for (std::vector<const Subgraph*>::const_iterator p(m_rules.begin());
       p != m_rules.end(); ++p) {
    delete *p;
  }
}

bool Node::IsPreterminal() const
{
  return (m_type == TREE
          && m_children.size() == 1
          && m_children[0]->m_type == TARGET);
}

void Node::PropagateIndex(int index)
{
  m_span.insert(index);
  for (std::vector<Node *>::const_iterator p(m_parents.begin());
       p != m_parents.end(); ++p) {
    (*p)->PropagateIndex(index);
  }
}

std::vector<std::string> Node::GetTargetWords() const
{
  std::vector<std::string> targetWords;
  GetTargetWords(targetWords);
  return targetWords;
}

void Node::GetTargetWords(std::vector<std::string> &targetWords) const
{
  if (m_type == TARGET) {
    targetWords.push_back(m_label);
  } else {
    for (std::vector<Node *>::const_iterator p(m_children.begin());
         p != m_children.end(); ++p) {
      (*p)->GetTargetWords(targetWords);
    }
  }
}

}  // namespace GHKM
}  // namespace Moses

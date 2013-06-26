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

#include "ParseTree.h"

namespace Moses
{
namespace GHKM
{

ParseTree::~ParseTree()
{
  for (std::vector<ParseTree*>::iterator p(m_children.begin());
       p != m_children.end(); ++p) {
    delete *p;
  }
}

void ParseTree::SetChildren(const std::vector<ParseTree*> &children)
{
  m_children = children;
}

void ParseTree::SetParent(ParseTree *parent)
{
  m_parent = parent;
}

void ParseTree::AddChild(ParseTree *child)
{
  m_children.push_back(child);
}

bool ParseTree::IsLeaf() const
{
  return m_children.empty();
}

}  // namespace GHKM
}  // namespace Moses

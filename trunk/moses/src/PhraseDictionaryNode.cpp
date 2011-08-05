// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#include "PhraseDictionaryNode.h"
#include "TargetPhrase.h"
#include "PhraseDictionaryMemory.h"

namespace Moses
{
PhraseDictionaryNode::~PhraseDictionaryNode()
{
  delete m_targetPhraseCollection;
}

void PhraseDictionaryNode::Sort(size_t tableLimit)
{
  // recusively sort
  NodeMap::iterator iter;
  for (iter = m_map.begin() ; iter != m_map.end() ; ++iter) {
    iter->second.Sort(tableLimit);
  }

  // sort TargetPhraseCollection in this node
  if (m_targetPhraseCollection != NULL)
    m_targetPhraseCollection->NthElement(tableLimit);
}

PhraseDictionaryNode *PhraseDictionaryNode::GetOrCreateChild(const Word &word)
{
  NodeMap::iterator iter = m_map.find(word);
  if (iter != m_map.end())
    return &iter->second;	// found it

  // can't find node. create a new 1
  return &(m_map[word] = PhraseDictionaryNode());
}

const PhraseDictionaryNode *PhraseDictionaryNode::GetChild(const Word &word) const
{
  NodeMap::const_iterator iter = m_map.find(word);
  if (iter != m_map.end())
    return &iter->second;	// found it

  // don't return anything
  return NULL;
}

}


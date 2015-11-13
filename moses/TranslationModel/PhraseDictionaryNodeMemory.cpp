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

#include "PhraseDictionaryNodeMemory.h"
#include "moses/TargetPhrase.h"
#include "moses/TranslationModel/PhraseDictionary.h"

using namespace std;

namespace Moses
{

void PhraseDictionaryNodeMemory::Prune(size_t tableLimit)
{
  // recusively prune
  for (TerminalMap::iterator p = m_sourceTermMap.begin(); p != m_sourceTermMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }
  for (NonTerminalMap::iterator p = m_nonTermMap.begin(); p != m_nonTermMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  m_targetPhraseCollection->Prune(true, tableLimit);
}

void PhraseDictionaryNodeMemory::Sort(size_t tableLimit)
{
  // recusively sort
  for (TerminalMap::iterator p = m_sourceTermMap.begin(); p != m_sourceTermMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }
  for (NonTerminalMap::iterator p = m_nonTermMap.begin(); p != m_nonTermMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  m_targetPhraseCollection->Sort(true, tableLimit);
}

PhraseDictionaryNodeMemory*
PhraseDictionaryNodeMemory::GetOrCreateChild(const Word &sourceTerm)
{
  return &m_sourceTermMap[sourceTerm];
}

#if defined(UNLABELLED_SOURCE)
PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetOrCreateNonTerminalChild(const Word &targetNonTerm)
{
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << targetNonTerm);

  return &m_nonTermMap[targetNonTerm];
}
#else
PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm)
{
  UTIL_THROW_IF2(!sourceNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << sourceNonTerm);
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << targetNonTerm);

  NonTerminalMapKey key(sourceNonTerm, targetNonTerm);
  return &m_nonTermMap[NonTerminalMapKey(sourceNonTerm, targetNonTerm)];
}
#endif

const PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetChild(const Word &sourceTerm) const
{
  UTIL_THROW_IF2(sourceTerm.IsNonTerminal(),
                 "Not a terminal: " << sourceTerm);

  TerminalMap::const_iterator p = m_sourceTermMap.find(sourceTerm);
  return (p == m_sourceTermMap.end()) ? NULL : &p->second;
}

#if defined(UNLABELLED_SOURCE)
const PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetNonTerminalChild(const Word &targetNonTerm) const
{
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << targetNonTerm);

  NonTerminalMap::const_iterator p = m_nonTermMap.find(targetNonTerm);
  return (p == m_nonTermMap.end()) ? NULL : &p->second;
}
#else
const PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetChild(const Word &sourceNonTerm, const Word &targetNonTerm) const
{
  UTIL_THROW_IF2(!sourceNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << sourceNonTerm);
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << targetNonTerm);

  NonTerminalMapKey key(sourceNonTerm, targetNonTerm);
  NonTerminalMap::const_iterator p = m_nonTermMap.find(key);
  return (p == m_nonTermMap.end()) ? NULL : &p->second;
}
#endif

void PhraseDictionaryNodeMemory::Remove()
{
  m_sourceTermMap.clear();
  m_nonTermMap.clear();
  m_targetPhraseCollection->Remove();
}

std::ostream& operator<<(std::ostream &out, const PhraseDictionaryNodeMemory &node)
{
  out << node.GetTargetPhraseCollection();
  return out;
}

TO_STRING_BODY(PhraseDictionaryNodeMemory)

}


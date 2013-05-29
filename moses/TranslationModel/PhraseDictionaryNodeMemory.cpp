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

PhraseDictionaryNodeMemory::~PhraseDictionaryNodeMemory()
{
  for (TerminalMap::iterator iter = m_sourceTermMap.begin(); iter != m_sourceTermMap.end(); ++iter) {
    const PhraseDictionaryNodeMemory *node = iter->second;
    delete node;
  }
  for (NonTerminalMap::iterator iter = m_nonTermMap.begin(); iter != m_nonTermMap.end(); ++iter) {
    const PhraseDictionaryNodeMemory *node = iter->second;
    delete node;
  }
  delete m_targetPhraseCollection;
}

TargetPhraseCollection &PhraseDictionaryNodeMemory::GetOrCreateTargetPhraseCollection()
{
  if (m_targetPhraseCollection == NULL)
    m_targetPhraseCollection = new TargetPhraseCollection();
  return *m_targetPhraseCollection;
}

void PhraseDictionaryNodeMemory::Prune(size_t tableLimit)
{
  // recusively prune
  for (TerminalMap::iterator p = m_sourceTermMap.begin(); p != m_sourceTermMap.end(); ++p) {
    p->second->Prune(tableLimit);
  }
  for (NonTerminalMap::iterator p = m_nonTermMap.begin(); p != m_nonTermMap.end(); ++p) {
    p->second->Prune(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  if (m_targetPhraseCollection != NULL)
    m_targetPhraseCollection->Prune(true, tableLimit);
}

void PhraseDictionaryNodeMemory::Sort(size_t tableLimit)
{
  // recusively sort
  for (TerminalMap::iterator p = m_sourceTermMap.begin(); p != m_sourceTermMap.end(); ++p) {
    p->second->Sort(tableLimit);
  }
  for (NonTerminalMap::iterator p = m_nonTermMap.begin(); p != m_nonTermMap.end(); ++p) {
    p->second->Sort(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  if (m_targetPhraseCollection != NULL) {
    m_targetPhraseCollection->Sort(true, tableLimit);
  }
}

PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetOrCreateChild(const Word &sourceTerm)
{
  //CHECK(!sourceTerm.IsNonTerminal());

  std::pair <TerminalMap::iterator,bool> insResult;
  PhraseDictionaryNodeMemory *node = new PhraseDictionaryNodeMemory();
  insResult = m_sourceTermMap.insert( std::make_pair(sourceTerm, node) );

  if (!insResult.second) {
    delete node;
  }

  const TerminalMap::iterator &iter = insResult.first;
  PhraseDictionaryNodeMemory &ret = *iter->second;
  return &ret;
}

PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm)
{
  CHECK(sourceNonTerm.IsNonTerminal());
  CHECK(targetNonTerm.IsNonTerminal());

  NonTerminalMapKey key(sourceNonTerm, targetNonTerm);
  std::pair <NonTerminalMap::iterator,bool> insResult;
  PhraseDictionaryNodeMemory *node = new PhraseDictionaryNodeMemory();

  insResult = m_nonTermMap.insert( std::make_pair(key, node) );

  if (!insResult.second) {
    delete node;
  }

  const NonTerminalMap::iterator &iter = insResult.first;
  PhraseDictionaryNodeMemory &ret = *iter->second;
  return &ret;
}

const PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetChild(const Word &sourceTerm) const
{
  CHECK(!sourceTerm.IsNonTerminal());

  TerminalMap::const_iterator p = m_sourceTermMap.find(sourceTerm);
  return (p == m_sourceTermMap.end()) ? NULL : p->second;
}

const PhraseDictionaryNodeMemory *PhraseDictionaryNodeMemory::GetChild(const Word &sourceNonTerm, const Word &targetNonTerm) const
{
  CHECK(sourceNonTerm.IsNonTerminal());
  CHECK(targetNonTerm.IsNonTerminal());

  NonTerminalMapKey key(sourceNonTerm, targetNonTerm);
  NonTerminalMap::const_iterator p = m_nonTermMap.find(key);
  return (p == m_nonTermMap.end()) ? NULL : p->second;
}

void PhraseDictionaryNodeMemory::Clear()
{
  m_sourceTermMap.clear();
  m_nonTermMap.clear();
  delete m_targetPhraseCollection;

}

std::ostream& operator<<(std::ostream &out, const PhraseDictionaryNodeMemory &node)
{
  out << node.GetTargetPhraseCollection();
  return out;
}

TO_STRING_BODY(PhraseDictionaryNodeMemory)

}


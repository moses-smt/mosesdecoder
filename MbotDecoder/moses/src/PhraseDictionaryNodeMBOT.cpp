// $Id: PhraseDictionaryNodeMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $

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

#include "PhraseDictionaryNodeMBOT.h"
#include "TargetPhrase.h"
#include "PhraseDictionary.h"

namespace Moses
{

PhraseDictionaryNodeMBOT::~PhraseDictionaryNodeMBOT()
{
  delete m_mbotTargetPhraseCollection;
}

//prunes target phrase collection in this node
void PhraseDictionaryNodeMBOT::PruneMBOT(size_t tableLimit)
{
  for (TerminalMapMBOT::iterator p = m_mbotSourceTermMap.begin(); p != m_mbotSourceTermMap.end(); ++p) {
    p->second.PruneMBOT(tableLimit);
  }
  for (NonTerminalMapMBOT::iterator p = m_mbotNonTermMap.begin(); p != m_mbotNonTermMap.end(); ++p) {
    p->second.PruneMBOT(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  if (m_mbotTargetPhraseCollection != NULL)
    m_mbotTargetPhraseCollection->Prune(true, tableLimit);
}

//prunes target phrase collection in this node
void PhraseDictionaryNodeMBOT::SortMBOT(size_t tableLimit)
{
  // recusively sort
  for (TerminalMapMBOT::iterator p = m_mbotSourceTermMap.begin(); p != m_mbotSourceTermMap.end(); ++p) {
    p->second.SortMBOT(tableLimit);
  }
  for (NonTerminalMapMBOT::iterator p = m_mbotNonTermMap.begin(); p != m_mbotNonTermMap.end(); ++p) {
    p->second.SortMBOT(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  if (m_mbotTargetPhraseCollection != NULL) {
    m_mbotTargetPhraseCollection->Sort(true, tableLimit);
  }
}

PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetOrCreateChildMBOT(const Word &sourceTerm)
{
  CHECK(!sourceTerm.IsNonTerminal());
  std::pair <TerminalMapMBOT::iterator,bool> insResult;
  insResult = m_mbotSourceTermMap.insert( std::make_pair(sourceTerm, PhraseDictionaryNodeMBOT()) );
  const TerminalMapMBOT::iterator &iter = insResult.first;
  PhraseDictionaryNodeMBOT &ret = iter->second;
  return &ret;

}

PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetOrCreateChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTermVec)
{
    CHECK(sourceNonTerm.IsNonTerminal());
    std::vector<Word> :: const_iterator itr_target;
    for(itr_target = targetNonTermVec.begin(); itr_target != targetNonTermVec.end(); itr_target++)
    {
        CHECK(itr_target->IsNonTerminal());
    }
    NonTerminalMapKeyMBOT key(sourceNonTerm, targetNonTermVec);
    std::pair <NonTerminalMapMBOT::iterator,bool> insResult;
    insResult = m_mbotNonTermMap.insert( std::make_pair(key, PhraseDictionaryNodeMBOT()) );
    const NonTerminalMapMBOT::iterator &iter = insResult.first;
    PhraseDictionaryNodeMBOT &ret = iter->second;
    return &ret;
}

const PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetChildMBOT(const Word &sourceTerm) const
{
  CHECK(!sourceTerm.IsNonTerminal());
  TerminalMapMBOT::const_iterator p = m_mbotSourceTermMap.find(sourceTerm);
  return (p == m_mbotSourceTermMap.end()) ? NULL : &p->second;
}

const PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTermVec) const
{
  CHECK(sourceNonTerm.IsNonTerminal());
  std::vector<Word> :: const_iterator itr_targetTerms;
  for(itr_targetTerms = targetNonTermVec.begin(); itr_targetTerms != targetNonTermVec.end(); itr_targetTerms++)
  {
        Word targetNonTerm = *itr_targetTerms;
        CHECK(targetNonTerm.IsNonTerminal());
  }
  NonTerminalMapKeyMBOT key(sourceNonTerm, targetNonTermVec);
  NonTerminalMapMBOT::const_iterator p = m_mbotNonTermMap.find(key);
  return (p == m_mbotNonTermMap.end()) ? NULL : &p->second;
}

void PhraseDictionaryNodeMBOT::ClearMBOT()
{
  m_mbotSourceTermMap.clear();
  m_mbotNonTermMap.clear();
  delete m_mbotTargetPhraseCollection;

}

std::ostream& operator<<(std::ostream &out, const PhraseDictionaryNodeMBOT &node)
{
    out << node.GetTargetPhraseCollectionMBOT();
  return out;
}

TO_STRING_BODY(PhraseDictionaryNodeMBOT)

}


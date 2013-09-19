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
   //std::cout << "DELETE PHRASE DICTIONARY NODE MBOT " << std::endl;
  delete m_mbotTargetPhraseCollection;
}

//prunes target phrase collection in this node
void PhraseDictionaryNodeMBOT::PruneMBOT(size_t tableLimit)
{
   //std::cout << "PRUNING TO LIMIT : " << tableLimit << std::endl;
  // recusively prune
  for (TerminalMapMBOT::iterator p = m_mbotSourceTermMap.begin(); p != m_mbotSourceTermMap.end(); ++p) {
    //FB : second element is phraseDictionaryNodeMBOT
    p->second.PruneMBOT(tableLimit);
  }
  for (NonTerminalMapMBOT::iterator p = m_mbotNonTermMap.begin(); p != m_mbotNonTermMap.end(); ++p) {
    //FB : second element is phraseDictionaryNodeMBOT
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
    //FB : second element is phraseDictionaryNodeMBOT
    p->second.SortMBOT(tableLimit);
  }
  for (NonTerminalMapMBOT::iterator p = m_mbotNonTermMap.begin(); p != m_mbotNonTermMap.end(); ++p) {
    //FB : second element is phraseDictionaryNodeMBOT
    p->second.SortMBOT(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  if (m_mbotTargetPhraseCollection != NULL) {
    m_mbotTargetPhraseCollection->Sort(true, tableLimit);
  }
}

PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetOrCreateChildMBOT(const Word &sourceTerm)
{

  //std::cout << "PDNM : Get or Create Child FOR TERMINAL : " << std::endl;
  CHECK(!sourceTerm.IsNonTerminal());
  std::pair <TerminalMapMBOT::iterator,bool> insResult;
  //make pair
  //std::cout << "KEY : SOURCE WORD : "<< sourceTerm << std::endl;
  insResult = m_mbotSourceTermMap.insert( std::make_pair(sourceTerm, PhraseDictionaryNodeMBOT()) );
  const TerminalMapMBOT::iterator &iter = insResult.first;
  PhraseDictionaryNodeMBOT &ret = iter->second;
  //std::cout << "ASSOCIATED NODE : " << &ret << std::endl;
  //FB : target phrase collection not yet done.
  //std::cout << "TPC " << ret.GetTargetPhraseCollection() << std::endl;
  return &ret;

}

PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetOrCreateChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTermVec)
{
    CHECK(sourceNonTerm.IsNonTerminal());
    //new : inserted for testing
    //if(targetNonTerm = NULL)
    //std::cout << "Target Phrase in Phrase Dict Node " << targetNonTerm << std::endl;
    //std::cout << "GETTING OR CREATING CHILD : " << sourceNonTerm << std::endl;
    std::vector<Word> :: const_iterator itr_target;
    //std::cout << "TARGET NON TERMS : " << std::endl;
    for(itr_target = targetNonTermVec.begin(); itr_target != targetNonTermVec.end(); itr_target++)
    {
        //std::cout << targetNonTerm << " ";
        CHECK(itr_target->IsNonTerminal());
    }

    //put vector of target Non terms into MBOT word object
    //MBOTWord * targetWord = new MBOTWord();
    //targetWord->SetWordVector(targetNonTermVec);

    //make const copy for inserting
    //const MBOTWord * targetWordCopy = targetWord;
    //targetWord = NULL;

    //std::cout << "NUMBER OF TARGET NON TERMS :  " << targetNonTermVec.size() << std::endl;
    //std::cout << "KEY : SOURCE WORD: " << sourceNonTerm << "TARGET WORDS: ";
    //NonTerminalMapKeyMBOT key(sourceNonTerm, *targetWordCopy);

    NonTerminalMapKeyMBOT key(sourceNonTerm, targetNonTermVec);
    std::pair <NonTerminalMapMBOT::iterator,bool> insResult;
    insResult = m_mbotNonTermMap.insert( std::make_pair(key, PhraseDictionaryNodeMBOT()) );
    const NonTerminalMapMBOT::iterator &iter = insResult.first;
    PhraseDictionaryNodeMBOT &ret = iter->second;
    //std::cout << "ASSOCIATED NODE : " << &ret << std::endl;
    return &ret;
}

const PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetChildMBOT(const Word &sourceTerm) const
{
  CHECK(!sourceTerm.IsNonTerminal());

  //std::cout << "PDNM : Looking for terminal child : " << sourceTerm << std::endl;
  TerminalMapMBOT::const_iterator p = m_mbotSourceTermMap.find(sourceTerm);
  //if(p == m_mbotSourceTermMap.end()){std::cout << "WORD NOT FOUND" << std::endl;};
  return (p == m_mbotSourceTermMap.end()) ? NULL : &p->second;
}

const PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTermVec) const
{
  CHECK(sourceNonTerm.IsNonTerminal());

  //std::cout << "PDNM : Looking for non terminal child : source " << sourceNonTerm;
  //new : check that each word in vector is a non-terminal
  std::vector<Word> :: const_iterator itr_targetTerms;
  for(itr_targetTerms = targetNonTermVec.begin(); itr_targetTerms != targetNonTermVec.end(); itr_targetTerms++)
  {
        Word targetNonTerm = *itr_targetTerms;
        //std::cout << "target " << *itr_targetTerms;
        CHECK(targetNonTerm.IsNonTerminal());
  }
  //EMPTYENDLINE
  //std::cout << std::endl;

  //std::cout << "Searching : " << sourceNonTerm << std::endl;
  //NonTerminalMapKeyMBOT key(sourceNonTerm, targetWord);
  NonTerminalMapKeyMBOT key(sourceNonTerm, targetNonTermVec);
  NonTerminalMapMBOT::const_iterator p = m_mbotNonTermMap.find(key);
  //if(p == m_mbotNonTermMap.end()){std::cout << "WORD NOT FOUND" << std::endl;}
  //std::cout << "PDNSCFG : return child" <<std::endl;
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

    //out << "Print node" << std::endl;
    out << node.GetTargetPhraseCollectionMBOT();

    //std::cout << "Getting target phrase collection "<< std::endl;
    //TargetPhraseCollection tpc = *node.GetTargetPhraseCollection();
    //std::cout << "Iterating over collection "<< std::endl;
    //size_t size = tpc.GetSize();
    //for(int ind =0; ind < size; ind++)
   //{
    //    const TargetPhrase * currentConst = tpc.GetTargetPhrase(ind);
    //    TargetPhrase * current = const_cast<TargetPhrase*>(currentConst);
    //    TargetPhraseMBOT * currentMBOT = static_cast<TargetPhraseMBOT*>(current);
    //    out << *currentMBOT << std::endl;

   //}
  return out;
}

TO_STRING_BODY(PhraseDictionaryNodeMBOT)

}


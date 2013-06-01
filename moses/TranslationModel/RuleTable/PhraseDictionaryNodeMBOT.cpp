//Fabienne Braune
//Phrase dictionary node for l-MBOT rules



#include "PhraseDictionaryNodeMBOT.h"
#include "moses/TargetPhrase.h"
#include "moses/TranslationModel/PhraseDictionary.h"

namespace Moses
{

PhraseDictionaryNodeMBOT::~PhraseDictionaryNodeMBOT()
{
   std::cout << "DELETE PHRASE DICTIONARY NODE MBOT " << std::endl;
   delete m_mbotTargetPhraseCollection;
}

//prunes target phrase collection in this node
void PhraseDictionaryNodeMBOT::PruneMBOT(size_t tableLimit)
{
  // recusively prune
  for (TerminalMapMBOT::iterator p = m_mbotSourceTermMap.begin(); p != m_mbotSourceTermMap.end(); ++p) {
    //Fabienne Braune : second element is phraseDictionaryNodeMBOT
    p->second.PruneMBOT(tableLimit);
  }
  for (NonTerminalMapMBOT::iterator p = m_mbotNonTermMap.begin(); p != m_mbotNonTermMap.end(); ++p) {
    //Fabienne Braune : second element is phraseDictionaryNodeMBOT
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
    //Fabienne Braune : second element is phraseDictionaryNodeMBOT
    p->second.SortMBOT(tableLimit);
  }
  for (NonTerminalMapMBOT::iterator p = m_mbotNonTermMap.begin(); p != m_mbotNonTermMap.end(); ++p) {
    ////Fabienne Braune : second element is phraseDictionaryNodeMBOT
    p->second.SortMBOT(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  if (m_mbotTargetPhraseCollection != NULL) {
	if(tableLimit == 0)
	{m_mbotTargetPhraseCollection->Sort(false, tableLimit);}
	else
    {m_mbotTargetPhraseCollection->Sort(true, tableLimit);}
  }
}

PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetOrCreateChildMBOT(const Word *sourceTerm)
{

  CHECK(!sourceTerm->IsNonTerminal());
  std::pair <TerminalMapMBOT::iterator,bool> insResult;
  insResult = m_mbotSourceTermMap.insert( std::make_pair(*sourceTerm, PhraseDictionaryNodeMBOT()) );
  const TerminalMapMBOT::iterator &iter = insResult.first;
  PhraseDictionaryNodeMBOT &ret = iter->second;
  return &ret;

}

PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetOrCreateChild(const Word sourceNonTerm, const WordSequence targetNonTermVec)
{

    CHECK(sourceNonTerm.IsNonTerminal());
    CHECK(targetNonTermVec.AreNonTerms());

    const NonTerminalMapKeyMBOT key(sourceNonTerm, targetNonTermVec);
    std::pair <NonTerminalMapMBOT::iterator,bool> insResult;
    insResult = m_mbotNonTermMap.insert(std::make_pair(key, PhraseDictionaryNodeMBOT()));
    const NonTerminalMapMBOT::iterator &iter = insResult.first;
    PhraseDictionaryNodeMBOT &ret = iter->second;
    return &ret;
}

const PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetChildMBOT(const Word * sourceTerm) const
{
  CHECK(!sourceTerm->IsNonTerminal());

  TerminalMapMBOT::const_iterator p = m_mbotSourceTermMap.find(*sourceTerm);
  return (p == m_mbotSourceTermMap.end()) ? NULL : &p->second;
}

const PhraseDictionaryNodeMBOT *PhraseDictionaryNodeMBOT::GetChild(const Word sourceNonTerm, const WordSequence targetNonTermVec) const
{
  CHECK(sourceNonTerm.IsNonTerminal());

  //Check that each target word is a non-terminal
  WordSequence:: const_iterator itr_targetTerms;
  for(itr_targetTerms = targetNonTermVec.begin(); itr_targetTerms != targetNonTermVec.end(); itr_targetTerms++)
  {
        CHECK(itr_targetTerms->IsNonTerminal());
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


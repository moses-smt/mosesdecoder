//Fabienne Braune
//Phrase dictionary node for l-MBOT rules

//Commments on implementation :
//Specificity of l-MBOT : a single source non-terminal can be associated to multiple target non-terminals

#pragma once

#include <map>
#include <vector>
#include <iterator>
#include <utility>
#include <ostream>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>
#include <boost/tuple/tuple.hpp>
#include "PhraseDictionaryNodeSCFG.h"
#include "moses/Word.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/WordSequence.h"

namespace Moses
{

class PhraseDictionarySCFG;
class PhraseDictionaryMBOT;


class PhraseDictionaryNodeMBOT : public PhraseDictionaryNodeSCFG
{
public:

 //Fabienne Braune : used stl maps
 //TODO : implement l-MBOT boost maps as in PhraseDictionaryNodeSCFG.cpp

 //Each source non-terminal is associated to several target non-terminals
 typedef std::pair<Word, const WordSequence> NonTerminalMapKeyMBOT;
 typedef std::map<Word, PhraseDictionaryNodeMBOT> TerminalMapMBOT;
 typedef std::map<const NonTerminalMapKeyMBOT, PhraseDictionaryNodeMBOT> NonTerminalMapMBOT;

private:
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionarySCFG&);
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryMBOT&);
  friend std::ostream& operator<<(std::ostream&, const TargetPhraseCollection&);

  friend class PhraseDictionaryMBOT;
  friend class std::map<Word, PhraseDictionaryNodeMBOT>;

protected:
  NonTerminalMapMBOT m_mbotNonTermMap;
  TerminalMapMBOT m_mbotSourceTermMap;
  TargetPhraseCollection *m_mbotTargetPhraseCollection;

  PhraseDictionaryNodeMBOT()
    : PhraseDictionaryNodeSCFG()
    , m_mbotTargetPhraseCollection(NULL)
  { }

public:
  ~PhraseDictionaryNodeMBOT();

  bool IsLeafMBOT() const {
    return m_mbotSourceTermMap.empty() && m_mbotNonTermMap.empty();
  }

  bool IsLeaf() const {
   std::cout << "Is leaf for non mbot terminal map NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const TargetPhraseCollection *GetTargetPhraseCollectionMBOT() const
  {
    return m_mbotTargetPhraseCollection;
  }

  TargetPhraseCollection &GetOrCreateTargetPhraseCollectionMBOT(Word sourceLHS) {

    if (m_mbotTargetPhraseCollection == NULL)
    {m_mbotTargetPhraseCollection = new TargetPhraseCollection();}
    return *m_mbotTargetPhraseCollection;
  }

  void Prune(size_t tableLimit)
  {
     std::cout << "Prune non mbot Phrase Dictionary NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  void PruneMBOT(size_t tableLimit);

  void Sort(size_t tableLimit)
  {
     std::cout << "Sort non mbot Phrase Dictionary NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  void SortMBOT(size_t tableLimit);

  PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceTerm)
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  PhraseDictionaryNodeMBOT *GetOrCreateChildMBOT(const Word * sourceTerm);

  PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm)
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  PhraseDictionaryNodeMBOT *GetOrCreateChild(const Word sourceNonTerm, const WordSequence targetNonTerm);

  const PhraseDictionaryNodeSCFG *GetChild(const Word &sourceTerm) const
  {
     std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const PhraseDictionaryNodeMBOT *GetChildMBOT(const Word *sourceTerm) const;

  const PhraseDictionaryNodeMBOT *GetChild(const Word sourceNonTerm, const Word targetNonTerm) const
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const PhraseDictionaryNodeMBOT *GetChild(const Word sourceNonTerm, const WordSequence targetNonTerm) const;

  const NonTerminalMap & GetNonTerminalMap() const
  {
     std::cout << "Get non mbot non term map NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const NonTerminalMapMBOT & GetNonTerminalMapMBOT() const {
    return m_mbotNonTermMap;
  }

  void Clear()
  {
     ClearMBOT();
  }

  void ClearMBOT();

  TO_STRING();
};

std::ostream& operator<<(std::ostream&, const PhraseDictionaryNodeMBOT&);

}

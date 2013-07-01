#pragma once

#include <map>
#include "Phrase.h"
#include "WordsRange.h"

namespace Moses
{

class PhraseDictionary;
class TargetPhraseCollection;

/** Each node contains
1. substring used to searching the phrase table
2. the source range it covers
3. a list of InputLatticeNode that it is a prefix of
This is for both sentence input, and confusion network/lattices
*/
class InputLatticeNode
{
protected:
  const InputLatticeNode *m_prevNode;
  Phrase m_phrase;
  WordsRange m_range;
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, void*> > m_targetPhrases;

public:
  InputLatticeNode()
    : m_prevNode(NULL)
    , m_range(NOT_FOUND, NOT_FOUND)
  {}
  InputLatticeNode(const Phrase &phrase, const WordsRange &range, const InputLatticeNode *prevNode)
    :m_prevNode(prevNode)
    ,m_phrase(phrase)
    ,m_range(range) {
  }

  const Phrase &GetPhrase() const {
    return m_phrase;
  }
  const WordsRange &GetWordsRange() const {
    return m_range;
  }

  void SetTargetPhrases(const PhraseDictionary &phraseDictionary
		  	  	  	  , const TargetPhraseCollection *targetPhrases
		  	  	  	  , void *ptNode) {
	  std::pair<const TargetPhraseCollection*, void*> value(targetPhrases, ptNode);
    m_targetPhrases[&phraseDictionary] = value;
  }
  const TargetPhraseCollection *GetTargetPhrases(const PhraseDictionary &phraseDictionary) const;

};

};


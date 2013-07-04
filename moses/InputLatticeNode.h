#pragma once

#include <map>
#include <iostream>
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
  friend std::ostream& operator<<(std::ostream& out, const InputLatticeNode &obj);

protected:
  const InputLatticeNode *m_prevNode;
  Phrase m_phrase;
  WordsRange m_range;
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> > m_targetPhrases;

public:
  explicit InputLatticeNode()
    : m_prevNode(NULL)
    , m_range(NOT_FOUND, NOT_FOUND) {
  }

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
  const InputLatticeNode *GetPrevNode() const {
    return m_prevNode;
  }

  void SetTargetPhrases(const PhraseDictionary &phraseDictionary
                        , const TargetPhraseCollection *targetPhrases
                        , const void *ptNode) {
    std::pair<const TargetPhraseCollection*, const void*> value(targetPhrases, ptNode);
    m_targetPhrases[&phraseDictionary] = value;
  }
  const TargetPhraseCollection *GetTargetPhrases(const PhraseDictionary &phraseDictionary) const;
  const void *GetPtNode(const PhraseDictionary &phraseDictionary) const;

};

};


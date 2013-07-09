#pragma once

#include <map>
#include <iostream>
#include "Phrase.h"
#include "WordsRange.h"

namespace Moses
{

class PhraseDictionary;
class TargetPhraseCollection;
class ScoreComponentCollection;

/** Each node contains
1. substring used to searching the phrase table
2. the source range it covers
3. a list of InputPath that it is a prefix of
This is for both sentence input, and confusion network/lattices
*/
class InputPath
{
  friend std::ostream& operator<<(std::ostream& out, const InputPath &obj);

protected:
  const InputPath *m_prevNode;
  Phrase m_phrase;
  WordsRange m_range;
  const ScoreComponentCollection *m_inputScore;
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> > m_targetPhrases;

public:
  explicit InputPath()
    : m_prevNode(NULL)
    , m_range(NOT_FOUND, NOT_FOUND)
    , m_inputScore(NULL) {
  }

  InputPath(const Phrase &phrase, const WordsRange &range, const InputPath *prevNode
		  ,const ScoreComponentCollection *inputScore);
  ~InputPath();

  const Phrase &GetPhrase() const {
    return m_phrase;
  }
  const WordsRange &GetWordsRange() const {
    return m_range;
  }
  const InputPath *GetPrevNode() const {
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
  const ScoreComponentCollection *GetInputScore() const
  { return m_inputScore; }

};

};


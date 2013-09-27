#pragma once

#include <map>
#include <iostream>
#include <list>
#include "Phrase.h"
#include "WordsRange.h"
#include "NonTerminal.h"

namespace Moses
{

class PhraseDictionary;
class TargetPhraseCollection;
class ScoreComponentCollection;
class TargetPhrase;
class InputPath;
struct ScorePair;

typedef std::list<InputPath*> InputPathList;

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
  const ScorePair *m_inputScore;
  const NonTerminalSet m_sourceNonTerms;

  // for phrase-based model only
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> > m_targetPhrases;

  // for syntax model onlu
  mutable std::vector<std::vector<const Word*> > m_ruleSourceFromInputPath;


  bool SetPlaceholders(TargetPhrase *targetPhrase) const;
public:
  explicit InputPath()
    : m_prevNode(NULL)
    , m_range(NOT_FOUND, NOT_FOUND)
    , m_inputScore(NULL) {
  }

  InputPath(const Phrase &phrase, const NonTerminalSet &sourceNonTerms, const WordsRange &range, const InputPath *prevNode
            ,const ScorePair *inputScore);
  ~InputPath();

  const Phrase &GetPhrase() const {
    return m_phrase;
  }
  const NonTerminalSet &GetNonTerminalSet() const {
    return m_sourceNonTerms;
  }
  const WordsRange &GetWordsRange() const {
    return m_range;
  }
  const Word &GetLastWord() const;

  const InputPath *GetPrevNode() const {
    return m_prevNode;
  }

  void SetTargetPhrases(const PhraseDictionary &phraseDictionary
                        , const TargetPhraseCollection *targetPhrases
                        , const void *ptNode);
  const TargetPhraseCollection *GetTargetPhrases(const PhraseDictionary &phraseDictionary) const;

  // pointer to internal node in phrase-table. Since this is implementation dependent, this is a void*
  const void *GetPtNode(const PhraseDictionary &phraseDictionary) const;
  const ScorePair *GetInputScore() const {
    return m_inputScore;
  }

  std::vector<const Word*> &AddRuleSourceFromInputPath() const {
    m_ruleSourceFromInputPath.push_back(std::vector<const Word*>());
    return m_ruleSourceFromInputPath.back();
  }

};

};


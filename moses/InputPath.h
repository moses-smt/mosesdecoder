#pragma once

#include <map>
#include <iostream>
#include <vector>
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

typedef std::vector<InputPath*> InputPathList;

/** Each node contains
1. substring used to searching the phrase table
2. the source range it covers
3. a list of InputPath that it is a prefix of
This is for both sentence input, and confusion network/lattices
*/
class InputPath
{
  friend std::ostream& operator<<(std::ostream& out, const InputPath &obj);

public:
  typedef std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> > TargetPhrases;

protected:
  const InputPath *m_prevPath;
  Phrase m_phrase;
  WordsRange m_range;
  const ScorePair *m_inputScore;
  size_t m_nextNode; // distance to next node. For lattices

  // for phrase-based model only
  TargetPhrases m_targetPhrases;

  // for syntax model only
  mutable std::vector<std::vector<const Word*> > m_ruleSourceFromInputPath;
  const NonTerminalSet m_sourceNonTerms;


public:
  explicit InputPath()
    : m_prevPath(NULL)
    , m_range(NOT_FOUND, NOT_FOUND)
    , m_inputScore(NULL)
    , m_nextNode(NOT_FOUND)
  {}

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

  const InputPath *GetPrevPath() const {
    return m_prevPath;
  }

  //! distance to next node in input lattice. For sentences and confusion networks, this should be 1 (default)
  size_t GetNextNode() const {
    return m_nextNode;
  }

  void SetNextNode(size_t nextNode) {
    m_nextNode = nextNode;
  }

  void SetTargetPhrases(const PhraseDictionary &phraseDictionary
                        , const TargetPhraseCollection *targetPhrases
                        , const void *ptNode);
  const TargetPhraseCollection *GetTargetPhrases(const PhraseDictionary &phraseDictionary) const;
  const TargetPhrases &GetTargetPhrases() const
  { return m_targetPhrases; }

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


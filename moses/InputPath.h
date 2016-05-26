// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#include <map>
#include <iostream>
#include <vector>
#include "Phrase.h"
#include "Range.h"
#include "NonTerminal.h"
#include "moses/FactorCollection.h"
#include <boost/shared_ptr.hpp>
#include "TargetPhraseCollection.h"
namespace Moses
{

class PhraseDictionary;
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

  typedef std::pair<TargetPhraseCollection::shared_ptr, const void*>
  TPCollStoreEntry;

  typedef std::map<const PhraseDictionary*, TPCollStoreEntry>
  TargetPhrases;

public:
  // ttaskwptr const ttask;
  TranslationTask const* ttask;
protected:
  const InputPath *m_prevPath;
  Phrase m_phrase;
  Range m_range;
  const ScorePair *m_inputScore;
  size_t m_nextNode; // distance to next node. For lattices

  // for phrase-based model only
  TargetPhrases m_targetPhrases;

  // for syntax model only
  mutable std::vector<std::vector<const Word*> > m_ruleSourceFromInputPath;
  const NonTerminalSet m_sourceNonTerms;
  std::vector<bool> m_sourceNonTermArray;


public:
  explicit InputPath()
    : m_prevPath(NULL)
    , m_range(NOT_FOUND, NOT_FOUND)
    , m_inputScore(NULL)
    , m_nextNode(NOT_FOUND) {
  }

  InputPath(TranslationTask const* ttask, // ttaskwptr const ttask,
            Phrase const& phrase,
            NonTerminalSet const& sourceNonTerms,
            Range const& range,
            InputPath const* prevNode,
            ScorePair const* inputScore);

  ~InputPath();

  const Phrase &GetPhrase() const {
    return m_phrase;
  }
  const NonTerminalSet &GetNonTerminalSet() const {
    return m_sourceNonTerms;
  }
  const std::vector<bool> &GetNonTerminalArray() const {
    return m_sourceNonTermArray;
  }
  const Range &GetWordsRange() const {
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

  void
  SetTargetPhrases(const PhraseDictionary &phraseDictionary,
                   TargetPhraseCollection::shared_ptr const& targetPhrases,
                   const void *ptNode);

  TargetPhraseCollection::shared_ptr
  GetTargetPhrases(const PhraseDictionary &phraseDictionary) const;

  const TargetPhrases &GetTargetPhrases() const {
    return m_targetPhrases;
  }

  // pointer to internal node in phrase-table. Since this is implementation dependent, this is a void*
  const void *GetPtNode(const PhraseDictionary &phraseDictionary) const;
  const ScorePair *GetInputScore() const {
    return m_inputScore;
  }

  size_t GetTotalRuleSize() const;

  std::vector<const Word*> &AddRuleSourceFromInputPath() const {
    m_ruleSourceFromInputPath.push_back(std::vector<const Word*>());
    return m_ruleSourceFromInputPath.back();
  }

};

};


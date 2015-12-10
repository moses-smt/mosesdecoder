
#pragma once

#include <boost/bimap.hpp>
#include "../PhraseDictionary.h"

class QueryEngine;
class target_text;

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

class ProbingPT : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const ProbingPT&);

public:
  ProbingPT(const std::string &line);
  ~ProbingPT();

  void Load(AllOptions::ptr const& opts);

  void InitializeForInput(ttasksptr const& ttask);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &,
    const ChartCellCollectionBase &,
    std::size_t);

  TO_STRING();


protected:
  QueryEngine *m_engine;

  typedef boost::bimap<const Factor *, uint64_t> SourceVocabMap;
  mutable SourceVocabMap m_sourceVocabMap;

  typedef boost::bimap<const Factor *, unsigned int> TargetVocabMap;
  mutable TargetVocabMap m_vocabMap;

  TargetPhraseCollection::shared_ptr CreateTargetPhrase(const Phrase &sourcePhrase) const;
  TargetPhrase *CreateTargetPhrase(const Phrase &sourcePhrase, const target_text &probingTargetPhrase) const;
  const Factor *GetTargetFactor(uint64_t probingId) const;
  uint64_t GetSourceProbingId(const Factor *factor) const;

  std::vector<uint64_t> ConvertToProbingSourcePhrase(const Phrase &sourcePhrase, bool &ok) const;

  uint64_t m_unkId;
};

}  // namespace Moses

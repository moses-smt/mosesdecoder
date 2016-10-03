
#pragma once
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/bimap.hpp>
#include "../PhraseDictionary.h"


namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;
class QueryEngine;
class target_text;

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
  uint64_t m_unkId;

  std::vector<uint64_t> m_sourceVocab; // factor id -> pt id
  std::vector<const Factor*> m_targetVocab; // pt id -> factor*
  std::vector<const AlignmentInfo*> m_aligns;

  boost::iostreams::mapped_file_source file;
  const char *data;

  void CreateAlignmentMap(const std::string path);

  TargetPhraseCollection::shared_ptr CreateTargetPhrase(const Phrase &sourcePhrase) const;
  TargetPhrase *CreateTargetPhrase(const Phrase &sourcePhrase, const target_text &probingTargetPhrase) const;
  const Factor *GetTargetFactor(uint64_t probingId) const;
  uint64_t GetSourceProbingId(const Factor *factor) const;

  std::vector<uint64_t> ConvertToProbingSourcePhrase(const Phrase &sourcePhrase, bool &ok) const;

};

}  // namespace Moses

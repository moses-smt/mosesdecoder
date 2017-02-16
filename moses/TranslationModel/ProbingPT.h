
#pragma once
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/bimap.hpp>
#include <boost/unordered_map.hpp>
#include "PhraseDictionary.h"
#include "util/mmap.hh"

namespace probingpt
{
class QueryEngine;
class target_text;
}

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

  void SetParameter(const std::string& key, const std::string& value);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &,
    const ChartCellCollectionBase &,
    std::size_t);

  TO_STRING();


protected:
  probingpt::QueryEngine *m_engine;
  uint64_t m_unkId;

  std::vector<uint64_t> m_sourceVocab; // factor id -> pt id
  std::vector<const Factor*> m_targetVocab; // pt id -> factor*
  std::vector<const AlignmentInfo*> m_aligns;
  util::LoadMethod load_method;

  boost::iostreams::mapped_file_source file;
  const char *data;

  // caching
  typedef boost::unordered_map<uint64_t, TargetPhraseCollection*> CachePb;
  CachePb m_cachePb;

  void CreateAlignmentMap(const std::string path);

  TargetPhraseCollection::shared_ptr CreateTargetPhrase(const Phrase &sourcePhrase) const;

  std::pair<bool, uint64_t> GetKey(const Phrase &sourcePhrase) const;
  void GetSourceProbingIds(const Phrase &sourcePhrase, bool &ok,
                           uint64_t probingSource[]) const;
  uint64_t GetSourceProbingId(const Word &word) const;
  uint64_t GetSourceProbingId(const Factor *factor) const;

  TargetPhraseCollection *CreateTargetPhrases(
    const Phrase &sourcePhrase, uint64_t key) const;
  TargetPhrase *CreateTargetPhrase(
    const char *&offset) const;

  inline const Factor *GetTargetFactor(uint32_t probingId) const {
    if (probingId >= m_targetVocab.size()) {
      return NULL;
    }
    return m_targetVocab[probingId];
  }

};

}  // namespace Moses

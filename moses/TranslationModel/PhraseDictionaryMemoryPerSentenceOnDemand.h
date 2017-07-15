
#pragma once

#include "PhraseDictionary.h"
#include "moses/TypeDef.h"
#include "moses/TranslationTask.h"

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

class PhraseDictionaryMemoryPerSentenceOnDemand : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryMemoryPerSentenceOnDemand&);

public:
  PhraseDictionaryMemoryPerSentenceOnDemand(const std::string &line);

  void Load(AllOptions::ptr const& opts);

  void InitializeForInput(ttasksptr const& ttask);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&, std::size_t);

  void SetParameter(const std::string& key, const std::string& value);

  TargetPhraseCollection::shared_ptr  GetTargetPhraseCollectionNonCacheLEGACY(const Phrase &source) const;

  TO_STRING();


protected:
  typedef boost::unordered_map<Phrase, TargetPhraseCollection::shared_ptr> Coll;
  mutable boost::thread_specific_ptr<Coll> m_coll;

  bool m_valuesAreProbabilities;

  Coll &GetColl() const;

};

}  // namespace Moses

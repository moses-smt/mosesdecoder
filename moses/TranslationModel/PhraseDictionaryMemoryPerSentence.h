
#pragma once

#include "PhraseDictionary.h"
#include "moses/TypeDef.h"
#include "moses/TranslationTask.h"

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

class PhraseDictionaryMemoryPerSentence : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryMemoryPerSentence&);

public:
  PhraseDictionaryMemoryPerSentence(const std::string &line);

  void Load(AllOptions::ptr const& opts);

  void InitializeForInput(ttasksptr const& ttask);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&, std::size_t);

  TO_STRING();


protected:
  typedef boost::unordered_map<Phrase, TargetPhraseCollection::shared_ptr> Coll;
  mutable boost::thread_specific_ptr<Coll> m_coll;

  Coll &GetColl() const;

};

}  // namespace Moses

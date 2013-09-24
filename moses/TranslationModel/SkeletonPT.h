
#pragma once

#include "PhraseDictionary.h"

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

class SkeletonPT : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const SkeletonPT&);

public:
  SkeletonPT(const std::string &line);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &phraseDictionaryQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&);

  TO_STRING();

};

}  // namespace Moses

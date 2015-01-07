
#pragma once

#include "PhraseDictionary.h"

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

class OOVPT : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const OOVPT&);

public:
  OOVPT(const std::string &line);

  void Load();

  void InitializeForInput(InputType const& source);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&, std::size_t);

  TO_STRING();

  TargetPhrase *CreateTargetPhrase(const Word &sourceWord) const;

protected:
};

}  // namespace Moses

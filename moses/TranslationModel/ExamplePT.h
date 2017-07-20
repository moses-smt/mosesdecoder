
#pragma once

#include "PhraseDictionary.h"

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

class ExamplePT : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const ExamplePT&);

public:
  ExamplePT(const std::string &line);

  void Load(AllOptions::ptr const& opts);

  void InitializeForInput(ttasksptr const& ttask);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&, std::size_t);

  TO_STRING();


protected:
  TargetPhrase *CreateTargetPhrase(const Phrase &sourcePhrase) const;
};

}  // namespace Moses

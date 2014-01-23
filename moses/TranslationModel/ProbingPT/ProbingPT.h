
#pragma once

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

  void Load();

  void InitializeForInput(InputType const& source);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&);

  TO_STRING();


protected:
  QueryEngine *m_engine;

  TargetPhraseCollection *CreateTargetPhrase(const Phrase &sourcePhrase) const;
  TargetPhrase *CreateTargetPhrase(const Phrase &sourcePhrase, const target_text &probingTargetPhrase) const;

};

}  // namespace Moses

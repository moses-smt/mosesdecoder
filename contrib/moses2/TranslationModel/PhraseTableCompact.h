#pragma once
#include "PhraseTable.h"

namespace Moses2
{
class PhraseTableCompact: public PhraseTable
{
public:
  PhraseTableCompact(size_t startInd, const std::string &line);
  virtual ~PhraseTableCompact();
  void Load(System &system);
  virtual void SetParameter(const std::string& key, const std::string& value);


  // scfg
  virtual void InitActiveChart(
      MemPool &pool,
      const SCFG::Manager &mgr,
      SCFG::InputPath &path) const;

  virtual void Lookup(
      MemPool &pool,
      const SCFG::Manager &mgr,
      size_t maxChartSpan,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

protected:
  friend class PhraseDecoder;
  //PhraseDecoder* m_phraseDecoder;

  virtual void LookupGivenNode(
      MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::ActiveChartEntry &prevEntry,
      const SCFG::Word &wordSought,
      const Moses2::Hypotheses *hypos,
      const Moses2::Range &subPhraseRange,
      SCFG::InputPath &outPath) const;

};

}

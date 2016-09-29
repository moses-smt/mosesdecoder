#include "PhraseTableCompact.h"

namespace Moses2
{
PhraseTableCompact::PhraseTableCompact(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
{
  ReadParameters();
}

PhraseTableCompact::~PhraseTableCompact()
{

}

void PhraseTableCompact::Load(System &system)
{

}

void PhraseTableCompact::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "blah") {

  }
  else {
    PhraseTable::SetParameter(key, value);
  }
}

// scfg
void PhraseTableCompact::InitActiveChart(
    MemPool &pool,
    const SCFG::Manager &mgr,
    SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}

void PhraseTableCompact::Lookup(
    MemPool &pool,
    const SCFG::Manager &mgr,
    size_t maxChartSpan,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}

void PhraseTableCompact::LookupGivenNode(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const SCFG::ActiveChartEntry &prevEntry,
    const SCFG::Word &wordSought,
    const Moses2::Hypotheses *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const
{
  UTIL_THROW2("Not implemented");
}

}

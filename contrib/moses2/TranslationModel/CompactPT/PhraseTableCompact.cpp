#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread/tss.hpp>
#include "PhraseTableCompact.h"
#include "PhraseDecoder.h"

using namespace std;
using namespace boost::algorithm;

namespace Moses2
{
bool PhraseTableCompact::s_inMemoryByDefault = false;

PhraseTableCompact::PhraseTableCompact(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
,m_inMemory(s_inMemoryByDefault)
,m_useAlignmentInfo(true)
,m_hash(10, 16)
,m_phraseDecoder(0)
{
  ReadParameters();
}

PhraseTableCompact::~PhraseTableCompact()
{

}

void PhraseTableCompact::Load(System &system)
{
  std::string tFilePath = m_path;

  std::string suffix = ".minphr";
  if (!ends_with(tFilePath, suffix)) tFilePath += suffix;
  if (!FileExists(tFilePath))
    throw runtime_error("Error: File " + tFilePath + " does not exist.");

  //m_phraseDecoder
  //= new PhraseDecoder(*this, &m_input, &m_output, GetNumScores());

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

#pragma once

#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "moses/InputType.h"
#include "moses/Syntax/KBestExtractor.h"
#include "moses/Syntax/Manager.h"
#include "moses/Syntax/SVertexStack.h"
#include "moses/Word.h"

#include "OovHandler.h"
#include "ParserCallback.h"
#include "PChart.h"
#include "SChart.h"

namespace Moses
{
namespace Syntax
{

struct SHyperedge;

namespace S2T
{

template<typename Parser>
class Manager : public Syntax::Manager
{
public:
  Manager(ttasksptr const& ttask);

  void Decode();

  // Get the SHyperedge for the 1-best derivation.
  const SHyperedge *GetBestSHyperedge() const;

  void ExtractKBest(
    std::size_t k,
    std::vector<boost::shared_ptr<KBestExtractor::Derivation> > &kBestList,
    bool onlyDistinct=false) const;

  void OutputDetailedTranslationReport(OutputCollector *collector) const;

private:
  void FindOovs(const PChart &, boost::unordered_set<Word> &, std::size_t);

  void InitializeCharts();

  void InitializeParsers(PChart &, std::size_t);

  void RecombineAndSort(const std::vector<SHyperedge*> &, SVertexStack &);

  void PrunePChart(const SChart::Cell &, PChart::Cell &);

  PChart m_pchart;
  SChart m_schart;
  boost::shared_ptr<typename Parser::RuleTrie> m_oovRuleTrie;
  std::vector<boost::shared_ptr<Parser> > m_parsers;
};

}  // S2T
}  // Syntax
}  // Moses

// Implementation
#include "Manager-inl.h"

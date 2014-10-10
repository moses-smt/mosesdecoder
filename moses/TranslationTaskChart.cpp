#include "TranslationTaskChart.h"
#include "moses/Sentence.h"
#include "moses/StaticData.h"
#include "moses/Incremental.h"
#include "moses/IOWrapper.h"
#include "moses/OutputCollector.h"

using namespace std;
using namespace Moses;

namespace Moses
{

TranslationTaskChart::TranslationTaskChart(InputType *source, IOWrapper &ioWrapper,
boost::shared_ptr<HypergraphOutput<ChartManager> > hypergraphOutput)
: m_source(source)
, m_ioWrapper(ioWrapper)
, m_hypergraphOutput(hypergraphOutput)
{}

TranslationTaskChart::~TranslationTaskChart() {
  delete m_source;
}

void TranslationTaskChart::Run() {
const StaticData &staticData = StaticData::Instance();
const size_t translationId = m_source->GetTranslationId();

VERBOSE(2,"\nTRANSLATING(" << translationId << "): " << *m_source);

if (staticData.GetSearchAlgorithm() == ChartIncremental) {
  Incremental::Manager manager(*m_source);
  const std::vector<search::Applied> &nbest = manager.ProcessSentence();
  if (!nbest.empty()) {
	m_ioWrapper.OutputBestHypo(nbest[0], translationId);
	if (staticData.IsDetailedTranslationReportingEnabled()) {
	  const Sentence &sentence = dynamic_cast<const Sentence &>(*m_source);
	  m_ioWrapper.OutputDetailedTranslationReport(&nbest[0], sentence, translationId);
	}
	if (staticData.IsDetailedTreeFragmentsTranslationReportingEnabled()) {
	  const Sentence &sentence = dynamic_cast<const Sentence &>(*m_source);
	  m_ioWrapper.OutputDetailedTreeFragmentsTranslationReport(&nbest[0], sentence, translationId);
	}
  } else {
	m_ioWrapper.OutputBestNone(translationId);
  }
  if (staticData.GetNBestSize() > 0)
	m_ioWrapper.OutputNBestList(nbest, translationId);
  return;
}

ChartManager manager(*m_source);
manager.ProcessSentence();

UTIL_THROW_IF2(staticData.UseMBR(), "Cannot use MBR");

// Output search graph in hypergraph format for Kenneth Heafield's lazy hypergraph decoder
if (m_hypergraphOutput.get()) {
  m_hypergraphOutput->Write(manager);
}


// 1-best
const ChartHypothesis *bestHypo = manager.GetBestHypothesis();
m_ioWrapper.OutputBestHypo(bestHypo, translationId);
IFVERBOSE(2) {
  PrintUserTime("Best Hypothesis Generation Time:");
}

if (!staticData.GetAlignmentOutputFile().empty()) {
  m_ioWrapper.OutputAlignment(translationId, bestHypo);
}

if (staticData.IsDetailedTranslationReportingEnabled()) {
  const Sentence &sentence = dynamic_cast<const Sentence &>(*m_source);
  m_ioWrapper.OutputDetailedTranslationReport(bestHypo, sentence, translationId);
}
if (staticData.IsDetailedTreeFragmentsTranslationReportingEnabled()) {
  const Sentence &sentence = dynamic_cast<const Sentence &>(*m_source);
  m_ioWrapper.OutputDetailedTreeFragmentsTranslationReport(bestHypo, sentence, translationId);
}
if (!staticData.GetOutputUnknownsFile().empty()) {
  m_ioWrapper.OutputUnknowns(manager.GetParser().GetUnknownSources(),
							 translationId);
}

//DIMw
if (staticData.IsDetailedAllTranslationReportingEnabled()) {
  const Sentence &sentence = dynamic_cast<const Sentence &>(*m_source);
  size_t nBestSize = staticData.GetNBestSize();
  std::vector<boost::shared_ptr<ChartKBestExtractor::Derivation> > nBestList;
  manager.CalcNBest(nBestSize, nBestList, staticData.GetDistinctNBest());
  m_ioWrapper.OutputDetailedAllTranslationReport(nBestList, manager, sentence, translationId);
}

// n-best
size_t nBestSize = staticData.GetNBestSize();
if (nBestSize > 0) {
  VERBOSE(2,"WRITING " << nBestSize << " TRANSLATION ALTERNATIVES TO " << staticData.GetNBestFilePath() << endl);
  std::vector<boost::shared_ptr<ChartKBestExtractor::Derivation> > nBestList;
  manager.CalcNBest(nBestSize, nBestList,staticData.GetDistinctNBest());
  m_ioWrapper.OutputNBestList(nBestList, translationId);
  IFVERBOSE(2) {
	PrintUserTime("N-Best Hypotheses Generation Time:");
  }
}

if (staticData.GetOutputSearchGraph()) {
  std::ostringstream out;
  manager.OutputSearchGraphMoses( out);
  OutputCollector *oc = m_ioWrapper.GetSearchGraphOutputCollector();
  UTIL_THROW_IF2(oc == NULL, "File for search graph output not specified");
  oc->Write(translationId, out.str());
}

IFVERBOSE(2) {
  PrintUserTime("Sentence Decoding Time:");
}
manager.CalcDecoderStatistics();
}

} // namespace



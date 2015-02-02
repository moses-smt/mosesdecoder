#include "TranslationTask.h"
#include "moses/StaticData.h"
#include "moses/Sentence.h"
#include "moses/IOWrapper.h"
#include "moses/TranslationAnalysis.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/InputType.h"
#include "moses/OutputCollector.h"
#include "moses/Incremental.h"
#include "mbr.h"

#include "moses/Syntax/F2S/RuleMatcherCallback.h"
#include "moses/Syntax/F2S/RuleMatcherHyperTree.h"
#include "moses/Syntax/S2T/Parsers/RecursiveCYKPlusParser/RecursiveCYKPlusParser.h"
#include "moses/Syntax/S2T/Parsers/Scope3Parser/Parser.h"
#include "moses/Syntax/T2S/RuleMatcherSCFG.h"

#include "util/exception.hh"

using namespace std;

namespace Moses
{

TranslationTask::TranslationTask(InputType* source, Moses::IOWrapper &ioWrapper)
  : m_source(source)
  , m_ioWrapper(ioWrapper)
{}

TranslationTask::~TranslationTask()
{
  delete m_source;
}

void TranslationTask::Run()
{
  // shorthand for "global data"
  const StaticData &staticData = StaticData::Instance();
  const size_t translationId = m_source->GetTranslationId();

  // input sentence
  Sentence sentence;

  // report wall time spent on translation
  Timer translationTime;
  translationTime.start();

  // report thread number
#if defined(WITH_THREADS) && defined(BOOST_HAS_PTHREADS)
  VERBOSE(2, "Translating line " << translationId << "  in thread id " << pthread_self() << endl);
#endif


  // execute the translation
  // note: this executes the search, resulting in a search graph
  //       we still need to apply the decision rule (MAP, MBR, ...)
  Timer initTime;
  initTime.start();

  // which manager
  BaseManager *manager;

  if (!staticData.IsChart()) {
    // phrase-based
    manager = new Manager(*m_source);
  } else if (staticData.GetSearchAlgorithm() == SyntaxF2S ||
             staticData.GetSearchAlgorithm() == SyntaxT2S) {
    // STSG-based tree-to-string / forest-to-string decoding (ask Phil Williams)
    typedef Syntax::F2S::RuleMatcherCallback Callback;
    typedef Syntax::F2S::RuleMatcherHyperTree<Callback> RuleMatcher;
    manager = new Syntax::F2S::Manager<RuleMatcher>(*m_source);
  } else if (staticData.GetSearchAlgorithm() == SyntaxS2T) {
    // new-style string-to-tree decoding (ask Phil Williams)
    S2TParsingAlgorithm algorithm = staticData.GetS2TParsingAlgorithm();
    if (algorithm == RecursiveCYKPlus) {
      typedef Syntax::S2T::EagerParserCallback Callback;
      typedef Syntax::S2T::RecursiveCYKPlusParser<Callback> Parser;
      manager = new Syntax::S2T::Manager<Parser>(*m_source);
    } else if (algorithm == Scope3) {
      typedef Syntax::S2T::StandardParserCallback Callback;
      typedef Syntax::S2T::Scope3Parser<Callback> Parser;
      manager = new Syntax::S2T::Manager<Parser>(*m_source);
    } else {
      UTIL_THROW2("ERROR: unhandled S2T parsing algorithm");
    }
  } else if (staticData.GetSearchAlgorithm() == SyntaxT2S_SCFG) {
    // SCFG-based tree-to-string decoding (ask Phil Williams)
    typedef Syntax::F2S::RuleMatcherCallback Callback;
    typedef Syntax::T2S::RuleMatcherSCFG<Callback> RuleMatcher;
    const TreeInput *tree = NULL;
    manager = new Syntax::T2S::Manager<RuleMatcher>(*tree);
  } else if (staticData.GetSearchAlgorithm() == ChartIncremental) {
    // Ken's incremental decoding
    manager = new Incremental::Manager(*m_source);
  } else {
    // original SCFG manager
    manager = new ChartManager(*m_source);
  }

  VERBOSE(1, "Line " << translationId << ": Initialize search took " << initTime << " seconds total" << endl);
  manager->Decode();

  // we are done with search, let's look what we got
  Timer additionalReportingTime;
  additionalReportingTime.start();

  manager->OutputBest(m_ioWrapper.GetSingleBestOutputCollector());

  // output word graph
  manager->OutputWordGraph(m_ioWrapper.GetWordGraphCollector());

  // output search graph
  manager->OutputSearchGraph(m_ioWrapper.GetSearchGraphOutputCollector());

  manager->OutputSearchGraphSLF();

  // Output search graph in hypergraph format for Kenneth Heafield's lazy hypergraph decoder
  manager->OutputSearchGraphHypergraph();

  additionalReportingTime.stop();

  additionalReportingTime.start();

  // output n-best list
  manager->OutputNBest(m_ioWrapper.GetNBestOutputCollector());

  //lattice samples
  manager->OutputLatticeSamples(m_ioWrapper.GetLatticeSamplesCollector());

  // detailed translation reporting
  manager->OutputDetailedTranslationReport(m_ioWrapper.GetDetailedTranslationCollector());

  manager->OutputDetailedTreeFragmentsTranslationReport(m_ioWrapper.GetDetailTreeFragmentsOutputCollector());

  //list of unknown words
  manager->OutputUnknowns(m_ioWrapper.GetUnknownsCollector());

  manager->OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector());

  // report additional statistics
  manager->CalcDecoderStatistics();
  VERBOSE(1, "Line " << translationId << ": Additional reporting took " << additionalReportingTime << " seconds total" << endl);
  VERBOSE(1, "Line " << translationId << ": Translation took " << translationTime << " seconds total" << endl);
  IFVERBOSE(2) {
    PrintUserTime("Sentence Decoding Time:");
  }

  delete manager;
}

}

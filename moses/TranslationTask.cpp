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

#include "moses/Syntax/S2T/Parsers/RecursiveCYKPlusParser/RecursiveCYKPlusParser.h"
#include "moses/Syntax/S2T/Parsers/Scope3Parser/Parser.h"

#include "util/exception.hh"

using namespace std;

namespace Moses
{

TranslationTask::TranslationTask(InputType* source, Moses::IOWrapper &ioWrapper, int pbOrChart)
: m_source(source)
, m_ioWrapper(ioWrapper)
, m_pbOrChart(pbOrChart)
{}

TranslationTask::~TranslationTask() {
  delete m_source;
}

void TranslationTask::Run()
{
	switch (m_pbOrChart)
	{
	case 1:
		RunPb();
		break;
	case 2:
		RunChart();
		break;
	default:
	    UTIL_THROW(util::Exception, "Unknown value: " << m_pbOrChart);
	}
}


void TranslationTask::RunPb()
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
  TRACE_ERR("Translating line " << translationId << "  in thread id " << pthread_self() << endl);
#endif


  // execute the translation
  // note: this executes the search, resulting in a search graph
  //       we still need to apply the decision rule (MAP, MBR, ...)
  Timer initTime;
  initTime.start();
  Manager *manager = new Manager(*m_source);
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

  //list of unknown words
  manager->OutputUnknowns(m_ioWrapper.GetUnknownsCollector());

  // report additional statistics
  manager->CalcDecoderStatistics();
  VERBOSE(1, "Line " << translationId << ": Additional reporting took " << additionalReportingTime << " seconds total" << endl);
  VERBOSE(1, "Line " << translationId << ": Translation took " << translationTime << " seconds total" << endl);
  IFVERBOSE(2) {
    PrintUserTime("Sentence Decoding Time:");
  }

  delete manager;
}


void TranslationTask::RunChart()
{
	const StaticData &staticData = StaticData::Instance();
	const size_t translationId = m_source->GetTranslationId();

	VERBOSE(2,"\nTRANSLATING(" << translationId << "): " << *m_source);

    if (staticData.UseS2TDecoder()) {
      S2TParsingAlgorithm algorithm = staticData.GetS2TParsingAlgorithm();
      if (algorithm == RecursiveCYKPlus) {
        typedef Syntax::S2T::EagerParserCallback Callback;
        typedef Syntax::S2T::RecursiveCYKPlusParser<Callback> Parser;
        DecodeS2T<Parser>();
      } else if (algorithm == Scope3) {
        typedef Syntax::S2T::StandardParserCallback Callback;
        typedef Syntax::S2T::Scope3Parser<Callback> Parser;
        DecodeS2T<Parser>();
      } else {
        UTIL_THROW2("ERROR: unhandled S2T parsing algorithm");
      }
      return;
    }

	if (staticData.GetSearchAlgorithm() == ChartIncremental) {
	  Incremental::Manager manager(*m_source);
	  manager.Decode();
	  manager.OutputBest(m_ioWrapper.GetSingleBestOutputCollector());
	  manager.OutputDetailedTranslationReport(m_ioWrapper.GetDetailedTranslationCollector());
      manager.OutputDetailedTreeFragmentsTranslationReport(m_ioWrapper.GetDetailTreeFragmentsOutputCollector());
	  manager.OutputNBest(m_ioWrapper.GetNBestOutputCollector());

	  return;
	}

	ChartManager manager(*m_source);
	manager.Decode();

	UTIL_THROW_IF2(staticData.UseMBR(), "Cannot use MBR");

	// Output search graph in hypergraph format for Kenneth Heafield's lazy hypergraph decoder
	manager.OutputSearchGraphHypergraph();

	// 1-best
	manager.OutputBest(m_ioWrapper.GetSingleBestOutputCollector());

	IFVERBOSE(2) {
	  PrintUserTime("Best Hypothesis Generation Time:");
	}

    manager.OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector());
    manager.OutputDetailedTranslationReport(m_ioWrapper.GetDetailedTranslationCollector());
    manager.OutputDetailedTreeFragmentsTranslationReport(m_ioWrapper.GetDetailTreeFragmentsOutputCollector());
	manager.OutputUnknowns(m_ioWrapper.GetUnknownsCollector());

	// n-best
	manager.OutputNBest(m_ioWrapper.GetNBestOutputCollector());

	manager.OutputSearchGraph(m_ioWrapper.GetSearchGraphOutputCollector());

	IFVERBOSE(2) {
	  PrintUserTime("Sentence Decoding Time:");
	}
	manager.CalcDecoderStatistics();
}

}

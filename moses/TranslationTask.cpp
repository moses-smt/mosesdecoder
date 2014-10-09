#include "TranslationTask.h"
#include "moses/StaticData.h"
#include "moses/Sentence.h"
#include "moses/IOWrapper.h"
#include "moses/TranslationAnalysis.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/InputType.h"
#include "moses/OutputCollector.h"
#include "mbr.h"

using namespace std;

namespace Moses
{

TranslationTask::TranslationTask(InputType* source, Moses::IOWrapper &ioWrapper,
                bool outputSearchGraphSLF,
                boost::shared_ptr<HypergraphOutput<Manager> > hypergraphOutput) :
  m_source(source),
  m_ioWrapper(ioWrapper),
  m_outputSearchGraphSLF(outputSearchGraphSLF),
  m_hypergraphOutput(hypergraphOutput)
{}

TranslationTask::~TranslationTask() {
  delete m_source;
}

void TranslationTask::Run() {
  // shorthand for "global data"
  const StaticData &staticData = StaticData::Instance();

  // input sentence
  Sentence sentence;

  // report wall time spent on translation
  Timer translationTime;
  translationTime.start();

  // report thread number
#if defined(WITH_THREADS) && defined(BOOST_HAS_PTHREADS)
  TRACE_ERR("Translating line " << m_source->GetTranslationId() << "  in thread id " << pthread_self() << endl);
#endif


  // execute the translation
  // note: this executes the search, resulting in a search graph
  //       we still need to apply the decision rule (MAP, MBR, ...)
  Timer initTime;
  initTime.start();
  Manager manager(*m_source,staticData.GetSearchAlgorithm());
  VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Initialize search took " << initTime << " seconds total" << endl);
  manager.ProcessSentence();

  // we are done with search, let's look what we got
  Timer additionalReportingTime;
  additionalReportingTime.start();

  // output word graph
  if (m_ioWrapper.GetWordGraphCollector()) {
    ostringstream out;
    fix(out,PRECISION);
    manager.GetWordGraph(m_source->GetTranslationId(), out);
    m_ioWrapper.GetWordGraphCollector()->Write(m_source->GetTranslationId(), out.str());
  }

  // output search graph
  if (m_ioWrapper.GetSearchGraphOutputCollector()) {
    ostringstream out;
    fix(out,PRECISION);
    manager.OutputSearchGraph(m_source->GetTranslationId(), out);
    m_ioWrapper.GetSearchGraphOutputCollector()->Write(m_source->GetTranslationId(), out.str());

#ifdef HAVE_PROTOBUF
    if (staticData.GetOutputSearchGraphPB()) {
      ostringstream sfn;
      sfn << staticData.GetParam("output-search-graph-pb")[0] << '/' << m_source->GetTranslationId() << ".pb" << ends;
      string fn = sfn.str();
      VERBOSE(2, "Writing search graph to " << fn << endl);
      fstream output(fn.c_str(), ios::trunc | ios::binary | ios::out);
      manager.SerializeSearchGraphPB(m_source->GetTranslationId(), output);
    }
#endif
  }

  // Output search graph in HTK standard lattice format (SLF)
  if (m_outputSearchGraphSLF) {
    stringstream fileName;
    fileName << staticData.GetParam("output-search-graph-slf")[0] << "/" << m_source->GetTranslationId() << ".slf";
    ofstream *file = new ofstream;
    file->open(fileName.str().c_str());
    if (file->is_open() && file->good()) {
      ostringstream out;
      fix(out,PRECISION);
      manager.OutputSearchGraphAsSLF(m_source->GetTranslationId(), out);
      *file << out.str();
      file -> flush();
    } else {
      TRACE_ERR("Cannot output HTK standard lattice for line " << m_source->GetTranslationId() << " because the output file is not open or not ready for writing" << endl);
    }
    delete file;
  }

  // Output search graph in hypergraph format for Kenneth Heafield's lazy hypergraph decoder
  if (m_hypergraphOutput.get()) {
    m_hypergraphOutput->Write(manager);
  }

  additionalReportingTime.stop();

  // apply decision rule and output best translation(s)
  if (m_ioWrapper.GetSingleBestOutputCollector()) {
    ostringstream out;
    ostringstream debug;
    fix(debug,PRECISION);

    // all derivations - send them to debug stream
    if (staticData.PrintAllDerivations()) {
      additionalReportingTime.start();
      manager.PrintAllDerivations(m_source->GetTranslationId(), debug);
      additionalReportingTime.stop();
    }

    Timer decisionRuleTime;
    decisionRuleTime.start();

    // MAP decoding: best hypothesis
    const Hypothesis* bestHypo = NULL;
    if (!staticData.UseMBR()) {
      bestHypo = manager.GetBestHypothesis();
      if (bestHypo) {
        if (StaticData::Instance().GetOutputHypoScore()) {
          out << bestHypo->GetTotalScore() << ' ';
        }
        if (staticData.IsPathRecoveryEnabled()) {
          OutputInput(out, bestHypo);
          out << "||| ";
        }
        if (staticData.GetParam("print-id").size() && Scan<bool>(staticData.GetParam("print-id")[0]) ) {
          out << m_source->GetTranslationId() << " ";
        }

	  if (staticData.GetReportSegmentation() == 2) {
	    manager.GetOutputLanguageModelOrder(out, bestHypo);
	  }
        OutputBestSurface(
          out,
          bestHypo,
          staticData.GetOutputFactorOrder(),
          staticData.GetReportSegmentation(),
          staticData.GetReportAllFactors());
        if (staticData.PrintAlignmentInfo()) {
          out << "||| ";
          OutputAlignment(out, bestHypo);
        }

        OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector(), m_source->GetTranslationId(), bestHypo);
        IFVERBOSE(1) {
          debug << "BEST TRANSLATION: " << *bestHypo << endl;
        }
      } else {
        VERBOSE(1, "NO BEST TRANSLATION" << endl);
      }

      out << endl;
    }

    // MBR decoding (n-best MBR, lattice MBR, consensus)
    else {
      // we first need the n-best translations
      size_t nBestSize = staticData.GetMBRSize();
      if (nBestSize <= 0) {
        cerr << "ERROR: negative size for number of MBR candidate translations not allowed (option mbr-size)" << endl;
        exit(1);
      }
      TrellisPathList nBestList;
      manager.CalcNBest(nBestSize, nBestList,true);
      VERBOSE(2,"size of n-best: " << nBestList.GetSize() << " (" << nBestSize << ")" << endl);
      IFVERBOSE(2) {
        PrintUserTime("calculated n-best list for (L)MBR decoding");
      }

      // lattice MBR
      if (staticData.UseLatticeMBR()) {
        if (m_ioWrapper.GetNBestOutputCollector()) {
          //lattice mbr nbest
          vector<LatticeMBRSolution> solutions;
          size_t n  = min(nBestSize, staticData.GetNBestSize());
          getLatticeMBRNBest(manager,nBestList,solutions,n);
          ostringstream out;
          OutputLatticeMBRNBest(out, solutions,m_source->GetTranslationId());
          m_ioWrapper.GetNBestOutputCollector()->Write(m_source->GetTranslationId(), out.str());
        } else {
          //Lattice MBR decoding
          vector<Word> mbrBestHypo = doLatticeMBR(manager,nBestList);
          OutputBestHypo(mbrBestHypo, m_source->GetTranslationId(), staticData.GetReportSegmentation(),
                         staticData.GetReportAllFactors(),out);
          IFVERBOSE(2) {
            PrintUserTime("finished Lattice MBR decoding");
          }
        }
      }

      // consensus decoding
      else if (staticData.UseConsensusDecoding()) {
        const TrellisPath &conBestHypo = doConsensusDecoding(manager,nBestList);
        OutputBestHypo(conBestHypo, m_source->GetTranslationId(),
                       staticData.GetReportSegmentation(),
                       staticData.GetReportAllFactors(),out);
        OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector(), m_source->GetTranslationId(), conBestHypo);
        IFVERBOSE(2) {
          PrintUserTime("finished Consensus decoding");
        }
      }

      // n-best MBR decoding
      else {
        const TrellisPath &mbrBestHypo = doMBR(nBestList);
        OutputBestHypo(mbrBestHypo, m_source->GetTranslationId(),
                       staticData.GetReportSegmentation(),
                       staticData.GetReportAllFactors(),out);
        OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector(), m_source->GetTranslationId(), mbrBestHypo);
        IFVERBOSE(2) {
          PrintUserTime("finished MBR decoding");
        }
      }
    }

    // report best translation to output collector
    m_ioWrapper.GetSingleBestOutputCollector()->Write(m_source->GetTranslationId(),out.str(),debug.str());

    decisionRuleTime.stop();
    VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Decision rule took " << decisionRuleTime << " seconds total" << endl);
  }

  additionalReportingTime.start();

  // output n-best list
  if (m_ioWrapper.GetNBestOutputCollector() && !staticData.UseLatticeMBR()) {
    TrellisPathList nBestList;
    ostringstream out;
    manager.CalcNBest(staticData.GetNBestSize(), nBestList,staticData.GetDistinctNBest());
    OutputNBest(out, nBestList, staticData.GetOutputFactorOrder(), m_source->GetTranslationId(),
                staticData.GetReportSegmentation());
    m_ioWrapper.GetNBestOutputCollector()->Write(m_source->GetTranslationId(), out.str());
  }

  //lattice samples
  if (m_ioWrapper.GetLatticeSamplesCollector()) {
    TrellisPathList latticeSamples;
    ostringstream out;
    manager.CalcLatticeSamples(staticData.GetLatticeSamplesSize(), latticeSamples);
    OutputNBest(out,latticeSamples, staticData.GetOutputFactorOrder(), m_source->GetTranslationId(),
                staticData.GetReportSegmentation());
    m_ioWrapper.GetLatticeSamplesCollector()->Write(m_source->GetTranslationId(), out.str());
  }

  // detailed translation reporting
  if (m_ioWrapper.GetDetailedTranslationCollector()) {
    ostringstream out;
    fix(out,PRECISION);
    TranslationAnalysis::PrintTranslationAnalysis(out, manager.GetBestHypothesis());
    m_ioWrapper.GetDetailedTranslationCollector()->Write(m_source->GetTranslationId(),out.str());
  }

  //list of unknown words
  if (m_ioWrapper.GetUnknownsCollector()) {
    const vector<const Phrase*>& unknowns = manager.getSntTranslationOptions()->GetUnknownSources();
    ostringstream out;
    for (size_t i = 0; i < unknowns.size(); ++i) {
      out << *(unknowns[i]);
    }
    out << endl;
    m_ioWrapper.GetUnknownsCollector()->Write(m_source->GetTranslationId(), out.str());
  }

  // report additional statistics
  manager.CalcDecoderStatistics();
  VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Additional reporting took " << additionalReportingTime << " seconds total" << endl);
  VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Translation took " << translationTime << " seconds total" << endl);
  IFVERBOSE(2) {
    PrintUserTime("Sentence Decoding Time:");
  }
}


}

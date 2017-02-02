/***********************************************************************
Relative Entropy-based Phrase table Pruning
Copyright (C) 2012 Wang Ling

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

/**
 * Moses main, for single-threaded and multi-threaded.
 **/

#include <exception>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

#include "Hypothesis.h"
#include "Manager.h"
#include "IOWrapper.h"
#include "StaticData.h"
#include "Util.h"
#include "Timer.h"
#include "ThreadPool.h"
#include "TranslationAnalysis.h"
#include "OutputCollector.h"
#include "RelativeEntropyCalc.h"
#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "util/random.hh"

#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif

using namespace std;
using namespace Moses;
using namespace MosesCmd;

namespace MosesCmd
{
// output floats with three significant digits
static const size_t PRECISION = 3;

/** Enforce rounding */
void fix(std::ostream& stream, size_t size)
{
  stream.setf(std::ios::fixed);
  stream.precision(size);
}

/** Translates a sentence.
  * - calls the search (Manager)
  * - applies the decision rule
  * - outputs best translation and additional reporting
  **/
class TranslationTask : public Task
{

public:

  TranslationTask(size_t lineNumber,
                  InputType* source, OutputCollector* searchGraphCollector) :
    m_source(source), m_lineNumber(lineNumber),
    m_searchGraphCollector(searchGraphCollector) {}

	/** Translate one sentence
   * gets called by main function implemented at end of this source file */
  void Run() {

    // report thread number
#if defined(WITH_THREADS) && defined(BOOST_HAS_PTHREADS)
    TRACE_ERR("Translating line " << m_lineNumber << "  in thread id " << pthread_self() << std::endl);
#endif

    // shorthand for "global data"
    const StaticData &staticData = StaticData::Instance();
    // input sentence
    Sentence sentence();
    // set translation system
    const TranslationSystem& system = staticData.GetTranslationSystem(TranslationSystem::DEFAULT);

    // execute the translation
    // note: this executes the search, resulting in a search graph
    //       we still need to apply the decision rule (MAP, MBR, ...)
    Manager manager(m_lineNumber, *m_source,staticData.GetSearchAlgorithm(), &system);
    manager.ProcessSentence();

    // output search graph
    if (m_searchGraphCollector) {
      ostringstream out;
      fix(out,PRECISION);

      vector<SearchGraphNode> searchGraph;
      manager.GetSearchGraph(searchGraph);
      out << RelativeEntropyCalc::CalcRelativeEntropy(m_lineNumber,searchGraph) << endl;
      m_searchGraphCollector->Write(m_lineNumber, out.str());

    }
    manager.CalcDecoderStatistics();
  }

  ~TranslationTask() {
    delete m_source;
  }

private:
  InputType* m_source;
  size_t m_lineNumber;
  OutputCollector* m_searchGraphCollector;
  std::ofstream *m_alignmentStream;

};

static void PrintFeatureWeight(const FeatureFunction* ff)
{

  size_t weightStart  = StaticData::Instance().GetScoreIndexManager().GetBeginIndex(ff->GetScoreBookkeepingID());
  size_t weightEnd  = StaticData::Instance().GetScoreIndexManager().GetEndIndex(ff->GetScoreBookkeepingID());
  for (size_t i = weightStart; i < weightEnd; ++i) {
    cout << ff->GetScoreProducerDescription(i-weightStart) <<  " " << ff->GetScoreProducerWeightShortName(i-weightStart) << " "
         << StaticData::Instance().GetAllWeights()[i] << endl;
  }
}


static void ShowWeights()
{
  fix(cout,6);
  const StaticData& staticData = StaticData::Instance();
  const TranslationSystem& system = staticData.GetTranslationSystem(TranslationSystem::DEFAULT);
  const vector<const StatelessFeatureFunction*>& slf =system.GetStatelessFeatureFunctions();
  const vector<const StatefulFeatureFunction*>& sff = system.GetStatefulFeatureFunctions();
  const vector<PhraseDictionaryFeature*>& pds = system.GetPhraseDictionaries();
  const vector<GenerationDictionary*>& gds = system.GetGenerationDictionaries();
  for (size_t i = 0; i < sff.size(); ++i) {
    PrintFeatureWeight(sff[i]);
  }
  for (size_t i = 0; i < slf.size(); ++i) {
    PrintFeatureWeight(slf[i]);
  }
  for (size_t i = 0; i < pds.size(); ++i) {
    PrintFeatureWeight(pds[i]);
  }
  for (size_t i = 0; i < gds.size(); ++i) {
    PrintFeatureWeight(gds[i]);
  }
}

} //namespace

/** main function of the command line version of the decoder **/
int main(int argc, char** argv)
{
  try {

    // echo command line, if verbose
    IFVERBOSE(1) {
      TRACE_ERR("command: ");
      for(int i=0; i<argc; ++i) TRACE_ERR(argv[i]<<" ");
      TRACE_ERR(endl);
    }

    // set number of significant decimals in output
    fix(cout,PRECISION);
    fix(cerr,PRECISION);

    // load all the settings into the Parameter class
    // (stores them as strings, or array of strings)
    Parameter* params = new Parameter();
    if (!params->LoadParam(argc,argv)) {
      params->Explain();
      exit(1);
    }


    // initialize all "global" variables, which are stored in StaticData
    // note: this also loads models such as the language model, etc.
    ResetUserTime();
    if (!StaticData::LoadDataStatic(params, argv[0])) {
      exit(1);
    }

    // setting "-show-weights" -> just dump out weights and exit
    if (params->isParamSpecified("show-weights")) {
      ShowWeights();
      exit(0);
    }

    // shorthand for accessing information in StaticData
    const StaticData& staticData = StaticData::Instance();


    //initialise random numbers
    rand_init();

    // set up read/writing class
    IOWrapper* ioWrapper = GetIOWrapper(staticData);
    if (!ioWrapper) {
      cerr << "Error; Failed to create IO object" << endl;
      exit(1);
    }

    // check on weights
    vector<float> weights = staticData.GetAllWeights();
    IFVERBOSE(2) {
      TRACE_ERR("The score component vector looks like this:\n" << staticData.GetScoreIndexManager());
      TRACE_ERR("The global weight vector looks like this:");
      for (size_t j=0; j<weights.size(); j++) {
        TRACE_ERR(" " << weights[j]);
      }
      TRACE_ERR("\n");
    }
    // every score must have a weight!  check that here:
    if(weights.size() != staticData.GetScoreIndexManager().GetTotalNumberOfScores()) {
      TRACE_ERR("ERROR: " << staticData.GetScoreIndexManager().GetTotalNumberOfScores() << " score components, but " << weights.size() << " weights defined" << std::endl);
      exit(1);
    }

    // setting lexicalized reordering setup
    PhraseBasedReorderingState::m_useFirstBackwardScore = false;


    auto_ptr<OutputCollector> outputCollector;
    outputCollector.reset(new OutputCollector());

#ifdef WITH_THREADS
    ThreadPool pool(staticData.ThreadCount());
#endif

    // main loop over set of input sentences
    InputType* source = NULL;
    size_t lineCount = 0;
    while(ReadInput(*ioWrapper,staticData.GetInputType(),source)) {
      IFVERBOSE(1) {
        ResetUserTime();
      }
      // set up task of translating one sentence
      TranslationTask* task =
        new TranslationTask(lineCount,source, outputCollector.get());
      // execute task
#ifdef WITH_THREADS
    pool.Submit(task);
#else
      task->Run();
      delete task;
#endif

      source = NULL; //make sure it doesn't get deleted
      ++lineCount;
    }

  // we are done, finishing up
#ifdef WITH_THREADS
    pool.Stop(true); //flush remaining jobs
#endif

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

#ifndef EXIT_RETURN
  //This avoids that destructors are called (it can take a long time)
  exit(EXIT_SUCCESS);
#else
  return EXIT_SUCCESS;
#endif
}

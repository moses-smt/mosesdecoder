// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (c) 2006 University of Edinburgh
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
			this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
			this list of conditions and the following disclaimer in the documentation
			and/or other materials provided with the distribution.
    * Neither the name of the University of Edinburgh nor the names of its contributors
			may be used to endorse or promote products derived from this software
			without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

// example file on how to use moses library

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

#include <exception>
#include <fstream>
#include "Main.h"
#include "moses/TranslationAnalysis.h"
#include "mbr.h"
#include "moses/IOWrapper.h"

#include "moses/FactorCollection.h"
#include "moses/HypergraphOutput.h"
#include "moses/Manager.h"
#include "moses/Phrase.h"
#include "moses/Util.h"
#include "moses/Timer.h"
#include "moses/Sentence.h"
#include "moses/ConfusionNet.h"
#include "moses/WordLattice.h"
#include "moses/TreeInput.h"
#include "moses/ThreadPool.h"
#include "moses/ChartManager.h"
#include "moses/ChartHypothesis.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/TranslationTaskChart.h"

#include "util/usage.hh"
#include "util/exception.hh"



using namespace std;
using namespace Moses;

int main(int argc, char* argv[])
{
  try {
    IFVERBOSE(1) {
      TRACE_ERR("command: ");
      for(int i=0; i<argc; ++i) TRACE_ERR(argv[i]<<" ");
      TRACE_ERR(endl);
    }

    IOWrapper::FixPrecision(cout);
    IOWrapper::FixPrecision(cerr);

    // load data structures
    Parameter parameter;
    if (!parameter.LoadParam(argc, argv)) {
      return EXIT_FAILURE;
    }

    const StaticData &staticData = StaticData::Instance();
    if (!StaticData::LoadDataStatic(&parameter, argv[0]))
      return EXIT_FAILURE;

    if (parameter.isParamSpecified("show-weights")) {
      ShowWeights();
      exit(0);
    }

    UTIL_THROW_IF2(!staticData.IsChart(), "Must be SCFG model");

    // set up read/writing class
    IOWrapper *ioWrapper = IOWrapper::GetIOWrapper(staticData);

    // check on weights
    const ScoreComponentCollection& weights = staticData.GetAllWeights();
    IFVERBOSE(2) {
      TRACE_ERR("The global weight vector looks like this: ");
      TRACE_ERR(weights);
      TRACE_ERR("\n");
    }

    boost::shared_ptr<HypergraphOutput<ChartManager> > hypergraphOutput; 
    if (staticData.GetOutputSearchGraphHypergraph()) {
      hypergraphOutput.reset(new HypergraphOutput<ChartManager>(3));
    }

    if (ioWrapper == NULL)
      return EXIT_FAILURE;

#ifdef WITH_THREADS
    ThreadPool pool(staticData.ThreadCount());
#endif

    // read each sentence & decode
    InputType *source=NULL;
    size_t lineCount = staticData.GetStartTranslationId();
    while(ioWrapper->ReadInput(*ioWrapper,staticData.GetInputType(),source)) {
      source->SetTranslationId(lineCount);
      IFVERBOSE(1)
      ResetUserTime();

      FeatureFunction::CallChangeSource(source);

      TranslationTaskChart *task = new TranslationTaskChart(source, *ioWrapper, hypergraphOutput);
      source = NULL;  // task will delete source
#ifdef WITH_THREADS
      pool.Submit(task);  // pool will delete task
#else
      task->Run();
      delete task;
#endif
      ++lineCount;
    }

#ifdef WITH_THREADS
    pool.Stop(true);  // flush remaining jobs
#endif

    delete ioWrapper;
    FeatureFunction::Destroy();

    IFVERBOSE(1)
    PrintUserTime("End.");

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  IFVERBOSE(1) util::PrintUsage(std::cerr);

#ifndef EXIT_RETURN
  //This avoids that detructors are called (it can take a long time)
  exit(EXIT_SUCCESS);
#else
  return EXIT_SUCCESS;
#endif
}


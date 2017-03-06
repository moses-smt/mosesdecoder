// $Id: MainMT.cpp 3045 2010-04-05 13:07:29Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#include "util/usage.hh"

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

#include "moses/IOWrapper.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Timer.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/TrainingTask.h"
#include "util/random.hh"

#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif

using namespace std;
using namespace Moses;

namespace Moses
{

void OutputFeatureWeightsForHypergraph(std::ostream &outputSearchGraphStream)
{
  outputSearchGraphStream.setf(std::ios::fixed);
  outputSearchGraphStream.precision(6);
  StaticData::Instance().GetAllWeights().Save(outputSearchGraphStream);
}


} //namespace

/** main function of the command line version of the decoder **/
int main(int argc, char const** argv)
{
  //setting in the Staticdata a link between the thread id of this process and a NULL tasksptr
  // StaticData::InstanceNonConst().SetTask();  // => moved into StaticData constructor

  try {

#ifdef HAVE_PROTOBUF
    GOOGLE_PROTOBUF_VERIFY_VERSION;
#endif

    // echo command line, if verbose
    IFVERBOSE(1) {
      TRACE_ERR("command: ");
      for(int i=0; i<argc; ++i) TRACE_ERR(argv[i]<<" ");
      TRACE_ERR(endl);
    }

    // set number of significant decimals in output
    FixPrecision(cout);
    FixPrecision(cerr);

    // load all the settings into the Parameter class
    // (stores them as strings, or array of strings)
    Parameter params;
    if (!params.LoadParam(argc,argv)) {
      exit(1);
    }


    // initialize all "global" variables, which are stored in StaticData
    // note: this also loads models such as the language model, etc.
    ResetUserTime();
    if (!StaticData::LoadDataStatic(&params, argv[0])) {
      exit(1);
    }

    // setting "-show-weights" -> just dump out weights and exit
    if (params.isParamSpecified("show-weights")) {
      ShowWeights();
      exit(0);
    }

    // shorthand for accessing information in StaticData
    const StaticData& staticData = StaticData::Instance();


    //initialise random numbers
    util::rand_init();

    // set up read/writing class
    IFVERBOSE(1) {
      PrintUserTime("Created input-output object");
    }
    AllOptions::ptr opts(new AllOptions(*StaticData::Instance().options()));
    boost::shared_ptr<IOWrapper> ioWrapper(new IOWrapper(*opts));
    if (ioWrapper == NULL) {
      cerr << "Error; Failed to create IO object" << endl;
      exit(1);
    }

    // check on weights
    const ScoreComponentCollection& weights = staticData.GetAllWeights();
    IFVERBOSE(2) {
      TRACE_ERR("The global weight vector looks like this: ");
      TRACE_ERR(weights);
      TRACE_ERR("\n");
    }

#ifdef WITH_THREADS
#pragma message ("Compiling with Threads.")
    ThreadPool pool(staticData.ThreadCount());
#endif

    // main loop over set of input sentences

    boost::shared_ptr<ContextScope> scope(new ContextScope);
    boost::shared_ptr<InputType> source;
    while ((source = ioWrapper->ReadInput()) != NULL) {
      IFVERBOSE(1) {
        ResetUserTime();
      }

      // set up task of training one sentence
      boost::shared_ptr<TrainingTask> task;
      task = TrainingTask::create(source, ioWrapper, scope);

      // execute task
#ifdef WITH_THREADS
      pool.Submit(task);
#else
      task->Run();
#endif
    }

    // we are done, finishing up
#ifdef WITH_THREADS
    pool.Stop(true); //flush remaining jobs
#endif

    FeatureFunction::Destroy();

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  IFVERBOSE(1) util::PrintUsage(std::cerr);

#ifndef EXIT_RETURN
  //This avoids that destructors are called (it can take a long time)
  exit(EXIT_SUCCESS);
#else
  return EXIT_SUCCESS;
#endif
}

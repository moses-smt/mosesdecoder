// Fork of Main.cpp, to simulate post-editing sessions.
// Written by Ulrich Germann.

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

#include <boost/program_options.hpp>
#include "util/usage.hh"
#include "util/exception.hh"
#include "moses/Util.h"

#ifdef PT_UG
#include "moses/TranslationModel/UG/mmsapt.h"
#include "moses/TranslationModel/UG/generic/program_options/ug_splice_arglist.h"
#endif

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

#include "moses/IOWrapper.h"
#include "moses/Hypothesis.h"
#include "moses/HypergraphOutput.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Timer.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/TranslationTask.h"

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
int main(int argc, char** argv)
{
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
    // By the way, having a static member function in an Moses-specific class
    // just to do 
    // cout.setf(ios::fixed);cout.precision(3);
    // doesn't make sense.
    // 1. It makes the program harder to understand. If I see
    //    cout.setf(ios::fixed);cout.precision(3);
    //    I know immediately what's going on. If I see, 
    //    IOWrapper::FixPrecision(cout); 
    //    I assume that something much more elaborate is going on (side effects?)
    //    and need to look it up. 
    //    After all, one reads programs in order to understand them.
    // 2. If you want to enforce a certain default behaviour on stdout,
    //    that should happen only once. Other components should not do that. 
    // 3. If you want to format locally, use
    //    string IOWrapper::score_default_formatting = "%.3f";
    //    cout << boost::format(IOWrapper::score_default_formatting) % my_variable;
    //    This even allows you to do pretty printing by default if you wish. 
    //    (Actually, the format variable should be constructed 
    //    from an int IOWrapper::score_output_default_precision itself.)
    // 4. If output speed is an issue and you are afraid of the overhead that 
    //    boost::format may add, don't use streams to begin with.
    // IOWrapper::FixPrecision(cout); 
    // IOWrapper::FixPrecision(cerr);
    cout.setf(ios::fixed); cout.precision(3);
    cout.setf(ios::fixed); cout.precision(3);

#ifdef PT_UG
    // see my notes on the moses support mailing list, this is a hack around
    // the misguided implementation of LoadParameter(), which should leave
    // arguments it doesn't know alone.
    vector<pair<string,int> > argfilter(4);
    argfilter[0] = std::make_pair(string("--spe-src"),1);
    argfilter[1] = std::make_pair(string("--spe-trg"),1);
    argfilter[2] = std::make_pair(string("--spe-aln"),1);

    char** my_args; int my_acnt;
    char** mo_args; int mo_acnt;
    filter_arguments(argc, argv, mo_acnt, &mo_args, my_acnt, &my_args, argfilter);

    ifstream spe_src,spe_trg,spe_aln;
    for (int i = 0; i < my_acnt; i += 2)
      {
        if (!strcmp(my_args[i],"--spe-src"))
          spe_src.open(my_args[i+1]);
        else if (!strcmp(my_args[i],"--spe-trg"))
          spe_trg.open(my_args[i+1]);
        else if (!strcmp(my_args[i],"--spe-aln"))
          spe_aln.open(my_args[i+1]);
      }
#endif

    // load all the settings into the Parameter class
    // (stores them as strings, or array of strings)
    Parameter params;
    if (!params.LoadParam(mo_acnt,mo_args)) {
      exit(1);
    }

    // initialize all "global" variables, which are stored in StaticData
    // note: this also loads models such as the language model, etc.
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
    srand(time(NULL));

    // set up read/writing class
    IOWrapper* ioWrapper = IOWrapper::GetIOWrapper(staticData);
    if (!ioWrapper) {
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

    boost::shared_ptr<HypergraphOutput<Manager> > hypergraphOutput; 
    boost::shared_ptr<HypergraphOutput<ChartManager> > hypergraphOutputChart;

    if (staticData.GetOutputSearchGraphHypergraph()) {
    	if (staticData.IsChart()) {
    		hypergraphOutputChart.reset(new HypergraphOutput<ChartManager>(PRECISION));
    	}
    	else {
    		hypergraphOutput.reset(new HypergraphOutput<Manager>(PRECISION));
    	}
    }

#ifdef WITH_THREADS
    ThreadPool pool(staticData.ThreadCount());
#endif

    // main loop over set of input sentences
    InputType* source = NULL;
    size_t lineCount = staticData.GetStartTranslationId();
    while(ioWrapper->ReadInput(staticData.GetInputType(),source)) {
      source->SetTranslationId(lineCount);
      IFVERBOSE(1) {
        ResetUserTime();
      }

      FeatureFunction::CallChangeSource(source);

      // set up task of translating one sentence
      TranslationTask* task;
      if (staticData.IsChart()) {
    	  // scfg
          task = new TranslationTask(source, *ioWrapper, hypergraphOutputChart);
      }
      else {
	// pb
	task = new TranslationTask(source, *ioWrapper,
				   staticData.GetOutputSearchGraphSLF(),
				   hypergraphOutput);
      }
      
      // execute task
#ifdef WITH_THREADS
#ifdef PT_UG
      if (my_acnt)
      {
	// simulated post-editing: always run single-threaded!
        task->Run();
        delete task;
        string src,trg,aln;
        UTIL_THROW_IF2(!getline(spe_src,src), "[" << HERE << "] "
                       << "missing update data for simulated post-editing.");
	UTIL_THROW_IF2(!getline(spe_trg,trg), "[" << HERE << "] "
		       << "missing update data for simulated post-editing.");
	UTIL_THROW_IF2(!getline(spe_aln,aln), "[" << HERE << "] "
		       << "missing update data for simulated post-editing.");
	BOOST_FOREACH (PhraseDictionary* pd, PhraseDictionary::GetColl())
	  {
	    Mmsapt* sapt = dynamic_cast<Mmsapt*>(pd);
	    if (sapt) sapt->add(src,trg,aln);
	    VERBOSE(1,"[" << HERE << " added src] " << src << endl);
	    VERBOSE(1,"[" << HERE << " added trg] " << trg << endl);
	    VERBOSE(1,"[" << HERE << " added aln] " << aln << endl);
	  }
      }
      else
#endif
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

    delete ioWrapper;
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

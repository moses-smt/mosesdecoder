// $Id: ExportInterface.cpp 3045 2010-04-05 13:07:29Z hieuhoang1972 $

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
 * Moses interface for main function, for single-threaded and multi-threaded.
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

#include "IOWrapper.h"
#include "Hypothesis.h"
#include "Manager.h"
#include "StaticData.h"
#include "TypeDef.h"
#include "Util.h"
#include "Timer.h"
#include "TranslationModel/PhraseDictionary.h"
#include "FF/StatefulFeatureFunction.h"
#include "FF/StatelessFeatureFunction.h"
#include "TranslationTask.h"

#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif

#ifdef PT_UG
#include <boost/foreach.hpp>
#include "TranslationModel/UG/mmsapt.h"
#include "TranslationModel/UG/generic/program_options/ug_splice_arglist.h"
#endif

#ifdef HAVE_XMLRPC_C
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include "server/Translator.h"
#include "server/Optimizer.h"
#include "server/Updater.h"
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


Parameter params;

//! run moses in server mode
int
run_as_server()
{
#ifdef HAVE_XMLRPC_C
  int port; params->SetParameter(port, "server-port", 8080);
  bool isSerial; params->SetParameter(isSerial, "serial", false);
  string logfile; params->SetParameter(logfile, "server-log", string(""));
  size_t num_threads; params->SetParameter(num_threads, "threads", 10);
  if (isSerial) VERBOSE(1,"Running server in serial mode." << endl);
  
  xmlrpc_c::registry myRegistry;
  
  xmlrpc_c::methodPtr const translator(new Translator(numThreads));
  xmlrpc_c::methodPtr const updater(new Updater);
  xmlrpc_c::methodPtr const optimizer(new Optimizer);
  
  myRegistry.addMethod("translate", translator);
  myRegistry.addMethod("updater", updater);
  myRegistry.addMethod("optimize", optimizer);
  
  xmlrpc_c::serverAbyss myAbyssServer(myRegistry, port, logfile);
  
  XVERBOSE(1,"Listening on port " << port << endl);
  if (isSerial) { while(1) myAbyssServer.runOnce(); } 
  else myAbyssServer.run();

  std::cerr << "xmlrpc_c::serverAbyss.run() returned but should not." << std::endl;
#else
  std::cerr << "Moses was compiled without server support." << endl;   
#endif
  return 1;

}

int
batch_run()
{
  // shorthand for accessing information in StaticData
  const StaticData& staticData = StaticData::Instance();

  //initialise random numbers
  srand(time(NULL));

  IFVERBOSE(1) PrintUserTime("Created input-output object");
    
  IOWrapper* ioWrapper = new IOWrapper(); // set up read/writing class
  UTIL_THROW_IF2(ioWrapper == NULL, "Error; Failed to create IO object"
		 << " [" << HERE << "]");
  
  // check on weights
  const ScoreComponentCollection& weights = staticData.GetAllWeights();
  IFVERBOSE(2) 
    {
      TRACE_ERR("The global weight vector looks like this: ");
      TRACE_ERR(weights);
      TRACE_ERR("\n");
    }

#ifdef WITH_THREADS
  ThreadPool pool(staticData.ThreadCount());
#endif

  // main loop over set of input sentences
  InputType* source = NULL;
  size_t lineCount = staticData.GetStartTranslationId();
  while(ioWrapper->ReadInput(staticData.GetInputType(), source)) 
    {
      source->SetTranslationId(lineCount);
      IFVERBOSE(1) ResetUserTime();
      
      FeatureFunction::CallChangeSource(source);
      
      // set up task of translating one sentence
      TranslationTask* task = new TranslationTask(source, *ioWrapper);

      // execute task
#ifdef WITH_THREADS
#ifdef PT_UG
      // simulated post-editing requires threads (within the dynamic phrase tables)
      // but runs all sentences serially, to allow updating of the bitext.
      bool spe = params.isParamSpecified("spe-src");
      if (spe) 
	{
	  // simulated post-editing: always run single-threaded!
	  task->Run();
	  delete task;
	  string src,trg,aln;
	  UTIL_THROW_IF2(!getline(*ioWrapper->spe_src,src), "[" << HERE << "] "
			 << "missing update data for simulated post-editing.");
	  UTIL_THROW_IF2(!getline(*ioWrapper->spe_trg,trg), "[" << HERE << "] "
			 << "missing update data for simulated post-editing.");
	  UTIL_THROW_IF2(!getline(*ioWrapper->spe_aln,aln), "[" << HERE << "] "
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
      else pool.Submit(task);
#else
      pool.Submit(task);

#endif
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

  IFVERBOSE(1) util::PrintUsage(std::cerr);
  
#ifndef EXIT_RETURN
  //This avoids that destructors are called (it can take a long time)
  exit(EXIT_SUCCESS);
#else
  return EXIT_SUCCESS;
#endif
}

/** Called by main function of the command line version of the decoder **/
int decoder_main(int argc, char** argv)
{
  try 
    {
      
#ifdef HAVE_PROTOBUF
      GOOGLE_PROTOBUF_VERIFY_VERSION;
#endif
      
      // echo command line, if verbose
      IFVERBOSE(1) 
	{
	  TRACE_ERR("command: ");
	  for(int i=0; i<argc; ++i) TRACE_ERR(argv[i]<<" ");
	  TRACE_ERR(endl);
	}

      // set number of significant decimals in output
      FixPrecision(cout);
      FixPrecision(cerr);

      // load all the settings into the Parameter class
      // (stores them as strings, or array of strings)
      if (!params.LoadParam(argc,argv)) 
	exit(1);

      // initialize all "global" variables, which are stored in StaticData
      // note: this also loads models such as the language model, etc.
      if (!StaticData::LoadDataStatic(&params, argv[0])) 
	exit(1);
    
      // setting "-show-weights" -> just dump out weights and exit
      if (params.isParamSpecified("show-weights")) 
	{
	  ShowWeights();
	  exit(0);
	}
    
      if (params.GetParam("server"))
	return run_as_server();
      else
	return batch_run();
    
    } 
  catch (const std::exception &e) 
    {
      std::cerr << "Exception: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
}


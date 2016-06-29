// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include "util/random.hh"
#include "util/usage.hh"

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif


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
#include "ExportInterface.h"

#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif

#ifdef PT_UG
#include <boost/foreach.hpp>
#include "TranslationModel/UG/mmsapt.h"
#include "TranslationModel/UG/generic/program_options/ug_splice_arglist.h"
#endif

#include "ExportInterface.h"

#ifdef HAVE_XMLRPC_C
#include "moses/server/Server.h"
#endif
#include <signal.h>

using namespace std;
using namespace Moses;

namespace Moses
{
//extern size_t g_numHypos;

void OutputFeatureWeightsForHypergraph(std::ostream &outputSearchGraphStream)
{
  outputSearchGraphStream.setf(std::ios::fixed);
  outputSearchGraphStream.precision(6);
  StaticData::Instance().GetAllWeights().Save(outputSearchGraphStream);
}
} //namespace Moses

SimpleTranslationInterface::SimpleTranslationInterface(const string &mosesIni): m_staticData(StaticData::Instance())
{
  if (!m_params.LoadParam(mosesIni)) {
    cerr << "Error; Cannot load parameters at " << mosesIni<<endl;
    exit(1);
  }
  ResetUserTime();
  if (!StaticData::LoadDataStatic(&m_params, mosesIni.c_str())) {
    cerr << "Error; Cannot load static data in file " << mosesIni<<endl;
    exit(1);
  }

  util::rand_init();

}

SimpleTranslationInterface::~SimpleTranslationInterface()
{}

//the simplified version of string input/output translation
string SimpleTranslationInterface::translate(const string &inputString)
{
  boost::shared_ptr<Moses::IOWrapper> ioWrapper(new IOWrapper(*StaticData::Instance().options()));
  // main loop over set of input sentences
  size_t sentEnd = inputString.rfind('\n'); //find the last \n, the input stream has to be appended with \n to be translated
  const string &newString = sentEnd != string::npos ? inputString : inputString + '\n';

  istringstream inputStream(newString);  //create the stream for the interface
  ioWrapper->SetInputStreamFromString(inputStream);
  ostringstream outputStream;
  ioWrapper->SetOutputStream2SingleBestOutputCollector(&outputStream);

  boost::shared_ptr<InputType> source = ioWrapper->ReadInput();
  if (!source) return "Error: Source==null!!!";
  IFVERBOSE(1) {
    ResetUserTime();
  }

  // set up task of translating one sentence
  boost::shared_ptr<TranslationTask> task
  = TranslationTask::create(source, ioWrapper);
  task->Run();

  string output = outputStream.str();
  //now trim the end whitespace
  const string whitespace = " \t\f\v\n\r";
  size_t end = output.find_last_not_of(whitespace);
  return output.erase(end + 1);
}

Moses::StaticData& SimpleTranslationInterface::getStaticData()
{
  return StaticData::InstanceNonConst();
}
void SimpleTranslationInterface::DestroyFeatureFunctionStatic()
{
  FeatureFunction::Destroy();
}


Parameter params;

void
signal_handler(int signum)
{
  if (signum == SIGALRM) {
    exit(0); // that's what we expected from the child process after forking
  } else if (signum == SIGTERM || signum == SIGKILL) {
    exit(0);
  } else {
    std::cerr << "Unexpected signal " << signum << std::endl;
    exit(signum);
  }
}

//! run moses in server mode
int
run_as_server()
{
#ifdef HAVE_XMLRPC_C
  if (params.GetParam("daemon")) {
    kill(getppid(),SIGALRM);
  }
  MosesServer::Server server(params);
  return server.run(); // actually: don't return. see Server::run()
#else
  UTIL_THROW2("Moses was compiled without xmlrpc-c. "
              << "No server functionality available.");
#endif
}

int
batch_run()
{
  // shorthand for accessing information in StaticData
  const StaticData& staticData = StaticData::Instance();

  //initialise random numbers
  util::rand_init();

  IFVERBOSE(1) PrintUserTime("Created input-output object");

  // set up read/writing class:
  boost::shared_ptr<IOWrapper> ioWrapper(new IOWrapper(*staticData.options()));
  UTIL_THROW_IF2(ioWrapper == NULL, "Error; Failed to create IO object"
                 << " [" << HERE << "]");

  // check on weights
  const ScoreComponentCollection& weights = staticData.GetAllWeights();
  IFVERBOSE(2) {
    TRACE_ERR("The global weight vector looks like this: ");
    TRACE_ERR(weights);
    TRACE_ERR("\n");
  }

#ifdef WITH_THREADS
  ThreadPool pool(staticData.ThreadCount());
#endif

  // using context for adaptation:
  // e.g., context words / strings from config file / cmd line
  std::string context_string;
  params.SetParameter(context_string,"context-string",string(""));

  // ... or weights for documents/domains from config file / cmd. line
  std::string context_weights;
  params.SetParameter(context_weights,"context-weights",string(""));

  // ... or the surrounding context (--context-window ...)
  size_t size_t_max = std::numeric_limits<size_t>::max();
  bool use_context_window = ioWrapper->GetLookAhead() || ioWrapper->GetLookBack();
  // bool use_context = use_context_window || context_string.size();
  bool use_sliding_context_window = (use_context_window
                                     && ioWrapper->GetLookAhead() != size_t_max);

  boost::shared_ptr<std::vector<std::string> >  context_window;
  boost::shared_ptr<std::vector<std::string> >* cw;
  cw = use_context_window ? &context_window : NULL;
  if (!cw && context_string.size())
    context_window.reset(new std::vector<std::string>(1,context_string));

  // global scope of caches, biases, etc., if any
  boost::shared_ptr<ContextScope> gscope;
  if (!use_sliding_context_window)
    gscope.reset(new ContextScope);

  // main loop over set of input sentences
  boost::shared_ptr<InputType> source;
  while ((source = ioWrapper->ReadInput(cw)) != NULL) {
    IFVERBOSE(1) ResetUserTime();

    // set up task of translating one sentence
    boost::shared_ptr<ContextScope>  lscope;
    if (gscope) lscope = gscope;
    else lscope.reset(new ContextScope);

    boost::shared_ptr<TranslationTask> task;
    task = TranslationTask::create(source, ioWrapper, lscope);

    if (cw) {
      if (context_string.size())
        context_window->push_back(context_string);
      if(!use_sliding_context_window)
        cw = NULL;
    }

    if (context_window)
      task->SetContextWindow(context_window);

    if (context_weights != "" && !task->GetScope()->GetContextWeights())
      task->GetScope()->SetContextWeights(context_weights);

    // Allow for (sentence-)context-specific processing prior to
    // decoding. This can be used, for example, for context-sensitive
    // phrase lookup.
    FeatureFunction::SetupAll(*task);

    // execute task
#ifdef WITH_THREADS
#ifdef PT_UG
    // simulated post-editing requires threads (within the dynamic phrase tables)
    // but runs all sentences serially, to allow updating of the bitext.
    bool spe = params.isParamSpecified("spe-src");
    if (spe) {
      // simulated post-editing: always run single-threaded!
      task->Run();
      string src,trg,aln;
      UTIL_THROW_IF2(!getline(*ioWrapper->spe_src,src), "[" << HERE << "] "
                     << "missing update data for simulated post-editing.");
      UTIL_THROW_IF2(!getline(*ioWrapper->spe_trg,trg), "[" << HERE << "] "
                     << "missing update data for simulated post-editing.");
      UTIL_THROW_IF2(!getline(*ioWrapper->spe_aln,aln), "[" << HERE << "] "
                     << "missing update data for simulated post-editing.");
      BOOST_FOREACH (PhraseDictionary* pd, PhraseDictionary::GetColl()) {
        Mmsapt* sapt = dynamic_cast<Mmsapt*>(pd);
        if (sapt) sapt->add(src,trg,aln);
        VERBOSE(1,"[" << HERE << " added src] " << src << endl);
        VERBOSE(1,"[" << HERE << " added trg] " << trg << endl);
        VERBOSE(1,"[" << HERE << " added aln] " << aln << endl);
      }
    } else pool.Submit(task);
#else
    pool.Submit(task);

#endif
#else
    task->Run();
#endif
  }

  // we are done, finishing up
#ifdef WITH_THREADS
  pool.Stop(true); //flush remaining jobs
#endif

//  cerr << "g_numHypos=" << Moses::g_numHypos << endl;

  FeatureFunction::Destroy();

  IFVERBOSE(0) util::PrintUsage(std::cerr);

#ifndef EXIT_RETURN
  //This avoids that destructors are called (it can take a long time)
  exit(EXIT_SUCCESS);
#else
  return EXIT_SUCCESS;
#endif
}

/** Called by main function of the command line version of the decoder **/
int decoder_main(int argc, char const** argv)
{
#ifdef NDEBUG
  try
#endif
  {
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
    if (!params.LoadParam(argc,argv))
      exit(1);

    // initialize all "global" variables, which are stored in StaticData
    // note: this also loads models such as the language model, etc.
    ResetUserTime();
    if (!StaticData::LoadDataStatic(&params, argv[0]))
      exit(1);

    //
#if 1
    pid_t pid;
    if (params.GetParam("daemon")) {
      pid = fork();
      if (pid) {
        pause();  // parent process
        exit(0);
      }
    }
#endif
    // setting "-show-weights" -> just dump out weights and exit
    if (params.isParamSpecified("show-weights")) {
      ShowWeights();
      exit(0);
    }

    if (params.GetParam("server")) {
      std::cerr << "RUN SERVER at pid " << pid << std::endl;
      return run_as_server();
    } else
      return batch_run();
  }
#ifdef NDEBUG
  catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
#endif
}


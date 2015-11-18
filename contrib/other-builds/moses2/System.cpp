/*
 * System.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <string>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "System.h"
#include "FF/FeatureFunction.h"
#include "TranslationModel/UnknownWordPenalty.h"
#include "legacy/Util2.h"
#include "util/exception.hh"

using namespace std;

System::System(const Parameter &paramsArg)
:params(paramsArg)
,featureFunctions(*this)

{
	ini_performance_options();
	params.SetParameter(stackSize, "stack", DEFAULT_MAX_HYPOSTACK_SIZE);
    params.SetParameter(maxDistortion, "distortion-limit", -1);
    params.SetParameter(maxPhraseLength, "max-phrase-length",
    		DEFAULT_MAX_PHRASE_LENGTH);
    params.SetParameter(searchAlgorithm, "search-algorithm", Normal);

	featureFunctions.Create();
	LoadWeights();
	featureFunctions.Load();
	LoadMappings();
}

System::~System() {
}

void System::LoadWeights()
{
  const PARAM_VEC *vec = params.GetParam("weight");
  UTIL_THROW_IF2(vec == NULL, "Must have [weight] section");

  weights.Init(featureFunctions);
  BOOST_FOREACH(const std::string &line, *vec) {
	  weights.CreateFromString(featureFunctions, line);
  }
}

void System::LoadMappings()
{
  const PARAM_VEC *vec = params.GetParam("mapping");
  UTIL_THROW_IF2(vec == NULL, "Must have [mapping] section");

  BOOST_FOREACH(const std::string &line, *vec) {
	  vector<string> toks = Tokenize(line);
	  assert( (toks.size() == 2 && toks[0] == "T") || (toks.size() == 3 && toks[1] == "T") );

	  size_t ptInd;
	  if (toks.size() == 2) {
		  ptInd = Scan<size_t>(toks[1]);
	  }
	  else {
		  ptInd = Scan<size_t>(toks[2]);
	  }
	  const PhraseTable *pt = featureFunctions.GetPhraseTablesExcludeUnknownWordPenalty(ptInd);
	  mappings.push_back(pt);
  }

  // unk pt
  const UnknownWordPenalty &unkWP = dynamic_cast<const UnknownWordPenalty&>(featureFunctions.FindFeatureFunction("UnknownWordPenalty0"));
  mappings.push_back(&unkWP);

}

MemPool &System::GetManagerPool() const
{
  MemPool *pool;
  pool = m_managerPool.get();
  if (pool == NULL) {
	pool = new MemPool;
	m_managerPool.reset(pool);
  }
  assert(pool);
  return *pool;
}

void
System
::ini_performance_options()
{
	  const PARAM_VEC *paramsVec;
	  // m_parameter->SetParameter<size_t>(m_timeout_threshold, "time-out", -1);
	  // m_timeout = (GetTimeoutThreshold() == (size_t)-1) ? false : true;

	  numThreads = 1;
	  paramsVec = params.GetParam("threads");
	  if (paramsVec && paramsVec->size()) {
	    if (paramsVec->at(0) == "all") {
	#ifdef WITH_THREADS
	      numThreads = boost::thread::hardware_concurrency();
	      if (!numThreads) {
	        std::cerr << "-threads all specified but Boost doesn't know how many cores there are";
	        throw;
	      }
	#else
	      std::cerr << "-threads all specified but moses not built with thread support";
	      return false;
	#endif
	    } else {
	    	numThreads = Scan<int>(paramsVec->at(0));
	      if (numThreads < 1) {
	        std::cerr << "Specify at least one thread.";
	        throw;
	      }
	#ifndef WITH_THREADS
	      if (numThreads > 1) {
	        std::cerr << "Error: Thread count of " << params->at(0)
	                  << " but moses not built with thread support";
	        throw
	      }
	#endif
	    }
	  }
	  return;
}

FactorCollection &System::GetVocab() const
{
	return m_vocab;
}

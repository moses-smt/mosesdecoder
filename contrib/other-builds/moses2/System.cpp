/*
 * System.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <string>
#include <iostream>
#include <boost/foreach.hpp>
#include "System.h"
#include "FF/FeatureFunction.h"
#include "TranslationModel/UnknownWordPenalty.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

System::System(const Moses::Parameter &paramsArg)
:params(paramsArg)
,featureFunctions(*this)

{
    params.SetParameter(stackSize, "stack", Moses::DEFAULT_MAX_HYPOSTACK_SIZE);

	featureFunctions.Create();
	LoadWeights();
	featureFunctions.Load();
	LoadMappings();
}

System::~System() {
}

void System::LoadWeights()
{
  const Moses::PARAM_VEC *vec = params.GetParam("weight");
  UTIL_THROW_IF2(vec == NULL, "Must have [weight] section");

  weights.Init(featureFunctions);
  BOOST_FOREACH(const std::string &line, *vec) {
	  weights.CreateFromString(featureFunctions, line);
  }
}

void System::LoadMappings()
{
  const Moses::PARAM_VEC *vec = params.GetParam("mapping");
  UTIL_THROW_IF2(vec == NULL, "Must have [mapping] section");

  BOOST_FOREACH(const std::string &line, *vec) {
	  vector<string> toks = Moses::Tokenize(line);
	  assert(toks.size() == 2);
	  assert(toks[0] == "T");
	  size_t ptInd = Moses::Scan<size_t>(toks[1]);
	  const PhraseTable *pt = featureFunctions.GetPhraseTablesExcludeUnknownWordPenalty(ptInd);
	  mappings.push_back(pt);
  }

  // unk pt
  const UnknownWordPenalty &unkWP = dynamic_cast<const UnknownWordPenalty&>(featureFunctions.FindFeatureFunction("UnknownWordPenalty0"));
  mappings.push_back(&unkWP);

}


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
#include "FF/TranslationModel/UnknownWordPenalty.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

System::System(const Moses::Parameter &params)
:m_featureFunctions(*this)
,m_params(params)
{
	m_featureFunctions.Create();
	LoadWeights();
	m_featureFunctions.Load();
	LoadMappings();
}

System::~System() {
}

void System::LoadWeights()
{
  const Moses::PARAM_VEC *params = m_params.GetParam("weight");
  UTIL_THROW_IF2(params == NULL, "Must have [weight] section");

  m_weights.Init(m_featureFunctions);
  BOOST_FOREACH(const std::string &line, *params) {
	  m_weights.CreateFromString(m_featureFunctions, line);
  }
}

void System::LoadMappings()
{
  const Moses::PARAM_VEC *params = m_params.GetParam("mapping");
  UTIL_THROW_IF2(params == NULL, "Must have [mapping] section");

  BOOST_FOREACH(const std::string &line, *params) {
	  vector<string> toks = Moses::Tokenize(line);
	  assert(toks.size() == 2);
	  assert(toks[0] == "T");
	  size_t ptInd = Moses::Scan<size_t>(toks[1]);
	  const PhraseTable *pt = m_featureFunctions.GetPhraseTablesExcludeUnknownWordPenalty(ptInd);
	  m_mappings.push_back(pt);
  }

  // unk pt
  const UnknownWordPenalty &unkWP = dynamic_cast<const UnknownWordPenalty&>(m_featureFunctions.FindFeatureFunction("UnknownWordPenalty0"));
  m_mappings.push_back(&unkWP);

}


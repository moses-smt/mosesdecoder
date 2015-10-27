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
#include "FeatureFunction.h"
#include "StatefulFeatureFunction.h"
#include "PhraseTable.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

System::System(const Moses::Parameter &params)
:m_ffStartInd(0)
,m_params(params)
{
	LoadFeatureFunctions();
}

System::~System() {
	Moses::RemoveAllInColl(m_featureFunctions);
}

void System::LoadFeatureFunctions()
{
  const Moses::PARAM_VEC *ffParams = m_params.GetParam("feature");
	UTIL_THROW_IF2(ffParams == NULL, "Must have [feature] section");

  BOOST_FOREACH(const std::string &line, *ffParams) {
	  cerr << "line=" << line << endl;
	  FeatureFunction *ff = FeatureFunction::Create(*this, line);

	  m_featureFunctions.push_back(ff);

	  StatefulFeatureFunction *sfff = dynamic_cast<StatefulFeatureFunction*>(ff);
	  if (sfff) {
		  m_statefulFeatureFunctions.push_back(sfff);
	  }

	  PhraseTable *pt = dynamic_cast<PhraseTable*>(ff);
	  if (pt) {
		  m_phraseTables.push_back(pt);
	  }
  }

  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  FeatureFunction *nonConstFF = const_cast<FeatureFunction*>(ff);
	  PhraseTable *pt = dynamic_cast<PhraseTable*>(nonConstFF);

	  if (pt) {
		  // do nothing. load pt last
	  }
	  else {
		  nonConstFF->Load(*this);
	  }
  }

  BOOST_FOREACH(const PhraseTable *pt, m_phraseTables) {
	  PhraseTable *nonConstPT = const_cast<PhraseTable*>(pt);
	  nonConstPT->Load(*this);
  }

  /*

	PhraseTable *pt = new PhraseTable(m_ffStartInd);
	pt->SetPtInd(m_phraseTables.size());
	pt->Load(*this);

	m_featureFunctions.push_back(pt);
	m_phraseTables.push_back(pt);
*/
}

/*
 * FeatureFunctions.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "FeatureFunctions.h"
#include "StatefulFeatureFunction.h"
#include "System.h"

#include "SkeletonStatelessFF.h"
#include "SkeletonStatefulFF.h"
#include "PhraseTableMemory.h"
#include "UnknownWordPenalty.h"
#include "WordPenalty.h"
#include "Distortion.h"

using namespace std;

FeatureFunctions::FeatureFunctions(System &system)
:m_system(system)
,m_ffStartInd(0)
{
	// TODO Auto-generated constructor stub

}

FeatureFunctions::~FeatureFunctions() {
	Moses::RemoveAllInColl(m_featureFunctions);
}

void FeatureFunctions::LoadFeatureFunctions()
{
  const Moses::Parameter &params = m_system.GetParameter();

  const Moses::PARAM_VEC *ffParams = params.GetParam("feature");
  UTIL_THROW_IF2(ffParams == NULL, "Must have [feature] section");

  BOOST_FOREACH(const std::string &line, *ffParams) {
	  cerr << "line=" << line << endl;
	  FeatureFunction *ff = Create(line);

	  m_featureFunctions.push_back(ff);

	  StatefulFeatureFunction *sfff = dynamic_cast<StatefulFeatureFunction*>(ff);
	  if (sfff) {
		  m_statefulFeatureFunctions.push_back(sfff);
	  }

	  PhraseTable *pt = dynamic_cast<PhraseTable*>(ff);
	  if (pt) {
		  pt->SetPtInd(m_phraseTables.size());
		  m_phraseTables.push_back(pt);
	  }

	  m_ffStartInd += ff->GetNumScores();
  }

  // load, everything but pts
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  FeatureFunction *nonConstFF = const_cast<FeatureFunction*>(ff);
	  PhraseTable *pt = dynamic_cast<PhraseTable*>(nonConstFF);

	  if (pt) {
		  // do nothing. load pt last
	  }
	  else {
		  cerr << "Loading " << nonConstFF->GetName() << endl;
		  nonConstFF->Load(m_system);
		  cerr << "Finished loading " << nonConstFF->GetName() << endl;
	  }
  }

  // load pt
  BOOST_FOREACH(const PhraseTable *pt, m_phraseTables) {
	  PhraseTable *nonConstPT = const_cast<PhraseTable*>(pt);
	  cerr << "Loading " << nonConstPT->GetName() << endl;
	  nonConstPT->Load(m_system);
	  cerr << "Finished loading " << nonConstPT->GetName() << endl;
  }
}

FeatureFunction *FeatureFunctions::Create(const std::string &line)
{
	vector<string> toks = Moses::Tokenize(line);

	FeatureFunction *ret;
	if (toks[0] == "PhraseDictionaryMemory") {
		ret = new PhraseTableMemory(m_ffStartInd, line);
	}
	else if (toks[0] == "UnknownWordPenalty") {
		ret = new UnknownWordPenalty(m_ffStartInd, line);
	}
	else if (toks[0] == "WordPenalty") {
		ret = new WordPenalty(m_ffStartInd, line);
	}
	else if (toks[0] == "Distortion") {
		ret = new Distortion(m_ffStartInd, line);
	}
	else {
		//ret = new SkeletonStatefulFF(m_ffStartInd, line);
		ret = new SkeletonStatelessFF(m_ffStartInd, line);
	}

	m_ffStartInd += ret->GetNumScores();
	return ret;
}

const FeatureFunction &FeatureFunctions::FindFeatureFunction(const std::string &name)
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  if (ff->GetName() == name) {
		  return *ff;
	  }
  }
  UTIL_THROW2(name << " not found");

}

void
FeatureFunctions::EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
        Scores& scores,
        Scores *estimatedFutureScores) const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  ff->EvaluateInIsolation(system, source, targetPhrase, scores, estimatedFutureScores);
  }

}

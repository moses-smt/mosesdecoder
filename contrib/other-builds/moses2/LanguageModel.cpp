/*
 * LanguageModel.cpp
 *
 *  Created on: 29 Oct 2015
 *      Author: hieu
 */

#include "LanguageModel.h"
#include "moses/Util.h"

LanguageModel::LanguageModel(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

LanguageModel::~LanguageModel() {
	// TODO Auto-generated destructor stub
}

void LanguageModel::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "factor") {
	  m_factorType = Moses::Scan<Moses::FactorType>(value);
  }
  else if (key == "order") {

  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

const Moses::FFState* LanguageModel::EmptyHypothesisState(const Manager &mgr, const Phrase &input) const
{

}

void
LanguageModel::EvaluateInIsolation(const System &system,
		  const PhraseBase &source, const TargetPhrase &targetPhrase,
        Scores &scores,
        Scores *estimatedFutureScores) const
{

}

Moses::FFState* LanguageModel::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const Moses::FFState &prevState,
  Scores &score) const
{

}

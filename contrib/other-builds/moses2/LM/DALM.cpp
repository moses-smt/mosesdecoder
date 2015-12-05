/*
 * DALM.cpp
 *
 *  Created on: 5 Dec 2015
 *      Author: hieu
 */

#include <contrib/other-builds/moses2/LM/DALM.h>

DALM::DALM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

DALM::~DALM() {
	// TODO Auto-generated destructor stub
}

void DALM::Load(System &system)
{

}

void DALM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "factor") {
	  m_factorType = Scan<FactorType>(value);
  }
  else if (key == "order") {
	  m_order = Scan<size_t>(value);
  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* DALM::BlankState(const Manager &mgr, const PhraseImpl &input) const
{

}

void DALM::EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const
{

}

 void DALM::EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
          Scores &scores,
          Scores *estimatedScores) const
 {

 }

 void DALM::EvaluateWhenApplied(const Manager &mgr,
    const Hypothesis &hypo,
    const FFState &prevState,
    Scores &scores,
	FFState &state) const
 {

 }


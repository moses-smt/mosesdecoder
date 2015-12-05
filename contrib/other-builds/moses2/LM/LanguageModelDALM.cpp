/*
 * LanguageModelDALM.cpp
 *
 *  Created on: 5 Dec 2015
 *      Author: hieu
 */

#include "LanguageModelDALM.h"
#include "dalm.h"

LanguageModelDALM::LanguageModelDALM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

LanguageModelDALM::~LanguageModelDALM() {
	// TODO Auto-generated destructor stub
}

void LanguageModelDALM::Load(System &system)
{

}

void LanguageModelDALM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
	m_factorType = Scan<FactorType>(value);
  } else if (key == "order") {
	m_nGramOrder = Scan<size_t>(value);
  } else if (key == "path") {
	m_filePath = value;
  } else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
  m_ContextSize = m_nGramOrder-1;
}

FFState* LanguageModelDALM::BlankState(const Manager &mgr, const PhraseImpl &input) const
{

}

void LanguageModelDALM::EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const
{

}

 void LanguageModelDALM::EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
          Scores &scores,
          Scores *estimatedScores) const
 {

 }

 void LanguageModelDALM::EvaluateWhenApplied(const Manager &mgr,
    const Hypothesis &hypo,
    const FFState &prevState,
    Scores &scores,
	FFState &state) const
 {

 }


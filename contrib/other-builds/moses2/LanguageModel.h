/*
 * LanguageModel.h
 *
 *  Created on: 29 Oct 2015
 *      Author: hieu
 */

#ifndef LANGUAGEMODEL_H_
#define LANGUAGEMODEL_H_

#include "StatefulFeatureFunction.h"
#include "moses/TypeDef.h"

class LanguageModel : public StatefulFeatureFunction
{
public:
	LanguageModel(size_t startInd, const std::string &line);
	virtual ~LanguageModel();

	virtual void SetParameter(const std::string& key, const std::string& value);

	  virtual const Moses::FFState* EmptyHypothesisState(const Manager &mgr, const Phrase &input) const;

	  virtual void
	  EvaluateInIsolation(const System &system,
			  const PhraseBase &source, const TargetPhrase &targetPhrase,
	          Scores &scores,
	          Scores *estimatedFutureScores) const;

	  virtual Moses::FFState* EvaluateWhenApplied(const Manager &mgr,
	    const Hypothesis &hypo,
	    const Moses::FFState &prevState,
	    Scores &score) const;

protected:
	std::string m_path;
	Moses::FactorType m_factorType;
};

#endif /* LANGUAGEMODEL_H_ */

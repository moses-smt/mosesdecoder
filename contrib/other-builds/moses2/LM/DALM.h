/*
 * DALM.h
 *
 *  Created on: 5 Dec 2015
 *      Author: hieu
 */

#pragma once
#include "../FF/StatefulFeatureFunction.h"
#include "../legacy/Util2.h"

class DALM : public StatefulFeatureFunction
{
public:
	DALM(size_t startInd, const std::string &line);
	virtual ~DALM();

	virtual void Load(System &system);
	virtual void SetParameter(const std::string& key, const std::string& value);

    virtual FFState* BlankState(const Manager &mgr, const PhraseImpl &input) const;
    virtual void EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const;

	  virtual void
	  EvaluateInIsolation(const System &system,
			  const Phrase &source, const TargetPhrase &targetPhrase,
	          Scores &scores,
	          Scores *estimatedScores) const;

	  virtual void EvaluateWhenApplied(const Manager &mgr,
	    const Hypothesis &hypo,
	    const FFState &prevState,
	    Scores &scores,
		FFState &state) const;

protected:
	std::string m_path;
	FactorType m_factorType;
	size_t m_order;

};


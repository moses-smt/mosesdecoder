/*
 * ContextFeature.h
 *
 *  Created on: Sep 18, 2013
 *      Author: braunefe
 */

#ifndef CONTEXTFEATURE_H_
#define CONTEXTFEATURE_H_

#pragma once

#include "moses/FF/StatelessFeatureFunction.h"


namespace Moses
{

class ContextFeature : public StatelessFeatureFunction
{
public:
	ContextFeature(const std::string &line)
	:StatelessFeatureFunction("ContextFeature", line)
	{}

	bool IsUseable(const FactorMask &mask) const
	{ return true; }

	void Evaluate(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const;

	//Fabienne Braune : Do nothing in here
	void Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ScoreComponentCollection &scoreBreakdown) const
	{}

    //Fabienne Braune : Call VW and re-score translation options
	void Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ChartTranslationOptions &transOpts) const;

  virtual void Evaluate(const Hypothesis& hypo,
	                        ScoreComponentCollection* accumulator) const
	  {}
	  void EvaluateChart(const ChartHypothesis &hypo,
	                             ScoreComponentCollection* accumulator) const
	  {}

};

}



#endif /* CONTEXTFEATURE_H_ */

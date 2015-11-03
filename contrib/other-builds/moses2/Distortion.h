/*
 * Distortion.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef DISTORTION_H_
#define DISTORTION_H_

#include "StatefulFeatureFunction.h"
#include "moses/Range.h"
#include "TypeDef.h"

class Distortion : public StatefulFeatureFunction
{
public:
	Distortion(size_t startInd, const std::string &line);
	virtual ~Distortion();

  virtual const Moses::FFState* EmptyHypothesisState(const Manager &mgr, const Phrase &input) const;

  virtual void
  EvaluateInIsolation(const System &system,
		  const PhraseBase &source,
		  const TargetPhrase &targetPhrase,
		  Scores &scores,
		  Scores *estimatedScore) const;

  virtual Moses::FFState* EvaluateWhenApplied(const Manager &mgr,
    const Hypothesis &hypo,
    const Moses::FFState &prevState,
    Scores &scores) const;

protected:
  SCORE CalculateDistortionScore(const Moses::Range &prev, const Moses::Range &curr, const int FirstGap) const;

  int ComputeDistortionDistance(const Moses::Range& prev, const Moses::Range& current) const;

};

#endif /* DISTORTION_H_ */

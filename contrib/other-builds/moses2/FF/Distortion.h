/*
 * Distortion.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef DISTORTION_H_
#define DISTORTION_H_

#include "StatefulFeatureFunction.h"
#include "../legacy/Range.h"
#include "../TypeDef.h"

class Distortion : public StatefulFeatureFunction
{
public:
	Distortion(size_t startInd, const std::string &line);
	virtual ~Distortion();

  virtual FFState* BlankState(const Manager &mgr, const PhraseImpl &input) const;
  virtual void EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const;

  virtual void
  EvaluateInIsolation(const System &system,
		  const Phrase &source,
		  const TargetPhrase &targetPhrase,
		  Scores &scores,
		  Scores *estimatedScores) const;

  virtual void EvaluateWhenApplied(const Recycler<Hypothesis*> &hypos) const
  {}

  virtual void EvaluateWhenApplied(const Manager &mgr,
    const Hypothesis &hypo,
    const FFState &prevState,
    Scores &scores,
	FFState &state) const;

  virtual void EvaluateWhenAppliedNonBatch(const Manager &mgr,
    const Hypothesis &hypo,
    const FFState &prevState,
    Scores &scores,
	FFState &state) const
  {
	  EvaluateWhenApplied(mgr, hypo, prevState, scores, state);
  }

protected:
  SCORE CalculateDistortionScore(const Range &prev, const Range &curr, const int FirstGap) const;

  int ComputeDistortionDistance(const Range& prev, const Range& current) const;

};

#endif /* DISTORTION_H_ */

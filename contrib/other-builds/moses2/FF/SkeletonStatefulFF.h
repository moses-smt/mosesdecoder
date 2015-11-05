/*
 * SkeletonStatefulFF.h
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#ifndef SKELETONSTATEFULFF_H_
#define SKELETONSTATEFULFF_H_

#include "StatefulFeatureFunction.h"

class SkeletonStatefulFF : public StatefulFeatureFunction
{
public:
	SkeletonStatefulFF(size_t startInd, const std::string &line);
	virtual ~SkeletonStatefulFF();

  virtual Moses::FFState* BlankState(const Manager &mgr, const PhraseImpl &input) const;
  virtual void EmptyHypothesisState(Moses::FFState &state, const Manager &mgr, const PhraseImpl &input) const;

  virtual void
  EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
		  Scores &scores,
		  Scores *estimatedScores) const;

  virtual Moses::FFState* EvaluateWhenApplied(const Manager &mgr,
    const Hypothesis &hypo,
    const Moses::FFState &prevState,
    Scores &scores) const;

};

#endif /* SKELETONSTATEFULFF_H_ */

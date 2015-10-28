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

  virtual void
  EvaluateInIsolation(const Manager &mgr,
		  const Phrase &source, const TargetPhrase &targetPhrase,
		  Scores& scores,
		  Scores& estimatedFutureScores) const;

};

#endif /* SKELETONSTATEFULFF_H_ */

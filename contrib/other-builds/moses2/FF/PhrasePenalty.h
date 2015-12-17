/*
 * SkeletonStatefulFF.h
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses2
{

class PhrasePenalty : public StatelessFeatureFunction
{
public:
	PhrasePenalty(size_t startInd, const std::string &line);
	virtual ~PhrasePenalty();

	  virtual void
	  EvaluateInIsolation(MemPool &pool,
			  const System &system,
			  const Phrase &source,
			  const TargetPhrase &targetPhrase,
			  Scores &scores,
			  Scores *estimatedScores) const;

};

}


/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include "PhrasePenalty.h"
#include "../Scores.h"

namespace Moses2
{

PhrasePenalty::PhrasePenalty(size_t startInd, const std::string &line)
:StatelessFeatureFunction(startInd, line)
{
	ReadParameters();
}

PhrasePenalty::~PhrasePenalty() {
	// TODO Auto-generated destructor stub
}

void
PhrasePenalty::EvaluateInIsolation(MemPool &pool,
		const System &system,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		SCORE *estimatedScore) const
{
  scores.PlusEquals(system, *this, 1);
}

}


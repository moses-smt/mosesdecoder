/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */
#include "../Scores.h"

#include "SkeletonStatelessFF.h"

namespace Moses2
{

SkeletonStatelessFF::SkeletonStatelessFF(size_t startInd, const std::string &line)
:StatelessFeatureFunction(startInd, line)
{
	ReadParameters();
}

SkeletonStatelessFF::~SkeletonStatelessFF() {
	// TODO Auto-generated destructor stub
}

void
SkeletonStatelessFF::EvaluateInIsolation(MemPool &pool,
		const System &system,
		const Phrase &source,
		const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedScores) const
{

}

}


/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include "SkeletonStatelessFF.h"

SkeletonStatelessFF::SkeletonStatelessFF(size_t startInd, const std::string &line)
:StatelessFeatureFunction(startInd, line)
{
	// TODO Auto-generated constructor stub

}

SkeletonStatelessFF::~SkeletonStatelessFF() {
	// TODO Auto-generated destructor stub
}

void
SkeletonStatelessFF::EvaluateInIsolation(const Manager &mgr,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores& scores,
		Scores& estimatedFutureScores) const
{

}

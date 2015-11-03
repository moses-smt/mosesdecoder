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
	//ReadParameters();
}

SkeletonStatelessFF::~SkeletonStatelessFF() {
	// TODO Auto-generated destructor stub
}

void
SkeletonStatelessFF::EvaluateInIsolation(const System &system,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedScore) const
{

}

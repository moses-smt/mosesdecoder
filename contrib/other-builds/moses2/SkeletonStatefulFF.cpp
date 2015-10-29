/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include "SkeletonStatefulFF.h"

SkeletonStatefulFF::SkeletonStatefulFF(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

SkeletonStatefulFF::~SkeletonStatefulFF() {
	// TODO Auto-generated destructor stub
}

void
SkeletonStatefulFF::EvaluateInIsolation(const System &system,
		const PhraseBase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedFutureScores) const
{

}

/*
 * WordPenalty.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include "WordPenalty.h"
#include "TypeDef.h"
#include "Scores.h"
#include "TargetPhrase.h"
#include "Manager.h"

WordPenalty::WordPenalty(size_t startInd, const std::string &line)
:StatelessFeatureFunction(startInd, line)
{
	// TODO Auto-generated constructor stub

}

WordPenalty::~WordPenalty() {
	// TODO Auto-generated destructor stub
}

void
WordPenalty::EvaluateInIsolation(const Manager &mgr,
		const Phrase &source,
		const TargetPhrase &targetPhrase,
		Scores& scores,
		Scores& estimatedFutureScores) const
{
  SCORE score = - (SCORE) targetPhrase.GetSize();
  scores.PlusEquals(mgr.GetSystem(), *this, score);

}



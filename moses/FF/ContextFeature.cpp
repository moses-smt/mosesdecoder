/*
 * ContextFeature.cpp
 *
 *  Created on: Sep 18, 2013
 *      Author: braunefe
 */

#include "ContextFeature.h"

namespace Moses
{

//override
void ContextFeature::Evaluate(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const
	{
		targetPhrase.SetRuleSource(source);


	}

void ContextFeature::Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ScoreComponentCollection &scoreBreakdown) const
	{


	}

}



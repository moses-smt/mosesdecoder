/*
 * StatefulFeatureFunction.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "StatefulFeatureFunction.h"
#include "../Search/Hypothesis.h"

StatefulFeatureFunction::StatefulFeatureFunction(size_t startInd, const std::string &line)
:FeatureFunction(startInd, line)
{
}

StatefulFeatureFunction::~StatefulFeatureFunction() {
	// TODO Auto-generated destructor stub
}

void StatefulFeatureFunction::EvaluateWhenApplied(const std::vector<Hypothesis*, MemPoolAllocator<Hypothesis*> > &hypos) const
{
	 BOOST_FOREACH(Hypothesis *hypo, hypos) {
		 hypo->EvaluateWhenApplied(*this);
	 }
}

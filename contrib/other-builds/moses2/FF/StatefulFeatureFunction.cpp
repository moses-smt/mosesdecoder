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

void StatefulFeatureFunction::EvaluateWhenApplied(const ObjectPoolContiguous<Hypothesis*> &hypos) const
{
	for (size_t i = 0; i < hypos.size(); ++i) {
		Hypothesis *hypo = hypos.get(i);
		 hypo->EvaluateWhenApplied(*this);
	 }
}

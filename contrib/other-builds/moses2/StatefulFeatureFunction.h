/*
 * StatefulFeatureFunction.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef STATEFULFEATUREFUNCTION_H_
#define STATEFULFEATUREFUNCTION_H_

#include "FeatureFunction.h"

class StatefulFeatureFunction : public FeatureFunction
{
public:
	StatefulFeatureFunction(size_t startInd, const std::string &line);
	virtual ~StatefulFeatureFunction();
};

#endif /* STATEFULFEATUREFUNCTION_H_ */

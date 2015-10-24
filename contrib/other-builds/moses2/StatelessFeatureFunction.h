/*
 * StatelessFeatureFunction.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef STATELESSFEATUREFUNCTION_H_
#define STATELESSFEATUREFUNCTION_H_

#include "FeatureFunction.h"

class StatelessFeatureFunction : public FeatureFunction
{
public:
	StatelessFeatureFunction(size_t startInd);
	virtual ~StatelessFeatureFunction();
};

#endif /* STATELESSFEATUREFUNCTION_H_ */

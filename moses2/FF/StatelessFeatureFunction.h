/*
 * StatelessFeatureFunction.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef STATELESSFEATUREFUNCTION_H_
#define STATELESSFEATUREFUNCTION_H_

#include "FeatureFunction.h"

namespace Moses2
{

class StatelessFeatureFunction: public FeatureFunction
{
public:
  StatelessFeatureFunction(size_t startInd, const std::string &line);
  virtual ~StatelessFeatureFunction();
};

}

#endif /* STATELESSFEATUREFUNCTION_H_ */

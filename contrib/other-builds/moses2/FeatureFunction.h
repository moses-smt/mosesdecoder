/*
 * FeatureFunction.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef FEATUREFUNCTION_H_
#define FEATUREFUNCTION_H_

#include <cstddef>

class FeatureFunction {
public:
	FeatureFunction(size_t startInd);
	virtual ~FeatureFunction();
	virtual void Load()
	{}

	size_t GetStartInd() const
	{ return m_startInd; }
	size_t GetNumScores() const
	{ return m_numScores; }

protected:
	size_t m_startInd;
	size_t m_numScores;
};

#endif /* FEATUREFUNCTION_H_ */

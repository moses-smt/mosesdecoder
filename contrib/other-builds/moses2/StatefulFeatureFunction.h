/*
 * StatefulFeatureFunction.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef STATEFULFEATUREFUNCTION_H_
#define STATEFULFEATUREFUNCTION_H_

#include "FeatureFunction.h"
#include "moses/FF/FFState.h"

class Hypothesis;

class StatefulFeatureFunction : public FeatureFunction
{
public:
	StatefulFeatureFunction(size_t startInd, const std::string &line);
	virtual ~StatefulFeatureFunction();

	void SetStatefulInd(size_t ind)
	{ m_statefulInd = ind; }
	size_t GetStatefulInd() const
	{ return m_statefulInd; }

	  //! return the state associated with the empty hypothesis for a given sentence
	  virtual const Moses::FFState* EmptyHypothesisState(const Manager &mgr, const Phrase &input) const = 0;

	  virtual Moses::FFState* EvaluateWhenApplied(const Manager &mgr,
	    const Hypothesis &hypo,
	    const Moses::FFState &prevState,
	    Scores &score) const = 0;

protected:
	  size_t m_statefulInd;
};

#endif /* STATEFULFEATUREFUNCTION_H_ */

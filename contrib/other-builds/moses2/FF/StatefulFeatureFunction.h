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

	  //! return uninitialise state
	  virtual Moses::FFState* BlankState(const Manager &mgr, const PhraseImpl &input) const = 0;

	  //! return the state associated with the empty hypothesis for a given sentence
	  virtual void EmptyHypothesisState(Moses::FFState &state, const Manager &mgr, const PhraseImpl &input) const = 0;

	  virtual Moses::FFState* EvaluateWhenApplied(const Manager &mgr,
	    const Hypothesis &hypo,
	    const Moses::FFState &prevState,
	    Scores &scores) const = 0;

protected:
	  size_t m_statefulInd;
};

#endif /* STATEFULFEATUREFUNCTION_H_ */

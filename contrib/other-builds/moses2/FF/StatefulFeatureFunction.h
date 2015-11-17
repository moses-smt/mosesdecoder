/*
 * StatefulFeatureFunction.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef STATEFULFEATUREFUNCTION_H_
#define STATEFULFEATUREFUNCTION_H_

#include "FeatureFunction.h"
#include "../legacy/FFState.h"
#include "../MemPool.h"

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
	  virtual FFState* BlankState(const Manager &mgr, const PhraseImpl &input) const = 0;

	  //! return the state associated with the empty hypothesis for a given sentence
	  virtual void EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const = 0;

	  virtual void EvaluateWhenApplied(const std::vector<Hypothesis*, MemPoolAllocator<Hypothesis*> > &hypos) const;

	  virtual void EvaluateWhenApplied(const Manager &mgr,
	    const Hypothesis &hypo,
	    const FFState &prevState,
	    Scores &scores,
		FFState &state) const = 0;

protected:
	  size_t m_statefulInd;
};

#endif /* STATEFULFEATUREFUNCTION_H_ */

/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include "SkeletonStatefulFF.h"
#include "../Search/Manager.h"

class SkeletonState : public Moses::FFState
{
public:
  int targetLen;

  SkeletonState()
  {
	  // uninitialised
  }

  virtual size_t hash() const {
    return (size_t) targetLen;
  }
  virtual bool operator==(const Moses::FFState& o) const {
    const SkeletonState& other = static_cast<const SkeletonState&>(o);
    return targetLen == other.targetLen;
  }

};

////////////////////////////////////////////////////////////////////////////////////////
SkeletonStatefulFF::SkeletonStatefulFF(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

SkeletonStatefulFF::~SkeletonStatefulFF() {
	// TODO Auto-generated destructor stub
}

Moses::FFState* SkeletonStatefulFF::BlankState(const Manager &mgr, const PhraseImpl &input) const
{
	MemPool &pool = mgr.GetPool();
    return new (pool.Allocate<SkeletonState>()) SkeletonState();
}

void SkeletonStatefulFF::EmptyHypothesisState(Moses::FFState &state, const Manager &mgr, const PhraseImpl &input) const
{
	SkeletonState &stateCast = static_cast<SkeletonState&>(state);
	stateCast.targetLen = 0;
}

void
SkeletonStatefulFF::EvaluateInIsolation(const System &system,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedScores) const
{
}

Moses::FFState* SkeletonStatefulFF::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const Moses::FFState &prevState,
  Scores &scores) const
{

}

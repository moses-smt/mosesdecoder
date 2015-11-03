/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include "SkeletonStatefulFF.h"

class SkeletonState : public Moses::FFState
{
  int m_targetLen;
public:
  SkeletonState(int targetLen)
    :m_targetLen(targetLen) {
  }

  virtual size_t hash() const {
    return (size_t) m_targetLen;
  }
  virtual bool operator==(const Moses::FFState& o) const {
    const SkeletonState& other = static_cast<const SkeletonState&>(o);
    return m_targetLen == other.m_targetLen;
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

const Moses::FFState* SkeletonStatefulFF::EmptyHypothesisState(const Manager &mgr, const PhraseImpl &input) const
{
    return new SkeletonState(0);
}

void
SkeletonStatefulFF::EvaluateInIsolation(const System &system,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedScore) const
{
}

Moses::FFState* SkeletonStatefulFF::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const Moses::FFState &prevState,
  Scores &scores) const
{

}

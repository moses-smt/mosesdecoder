/*
 * Hypothesis.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include "Hypothesis.h"
#include "Manager.h"
#include "System.h"

Hypothesis::Hypothesis(Manager &mgr,
		const TargetPhrase &tp,
		const Moses::Range &range,
		const Moses::Bitmap &bitmap)
:m_mgr(mgr)
,m_targetPhrase(tp)
,m_bitmap(bitmap)
,m_range(range)
,m_prevHypo(NULL)
{
	util::Pool &pool = mgr.GetPool();
	size_t numStatefulFFs = mgr.GetSystem().GetStatefulFeatureFunctions().size();
	m_ffStates = (Moses::FFState **) pool.Allocate(sizeof(Moses::FFState*) * numStatefulFFs);
}

Hypothesis::Hypothesis(const Hypothesis &prevHypo,
		const TargetPhrase &tp,
		const Moses::Range &pathRange,
		const Moses::Bitmap &bitmap)
:m_mgr(prevHypo.m_mgr)
,m_targetPhrase(tp)
,m_bitmap(bitmap)
,m_range(pathRange)
,m_prevHypo(&prevHypo)
{

}

Hypothesis::~Hypothesis() {
	// TODO Auto-generated destructor stub
}

size_t Hypothesis::hash() const
{
  size_t numStatefulFFs = m_mgr.GetSystem().GetStatefulFeatureFunctions().size();
  size_t seed;

  // coverage
  //seed = m_sourceCompleted.hash();

  // states
  for (size_t i = 0; i < numStatefulFFs; ++i) {
	const Moses::FFState *state = m_ffStates[i];
	size_t hash = state->hash();
	boost::hash_combine(seed, hash);
  }
  return seed;

}

bool Hypothesis::operator==(const Hypothesis &other) const
{
	size_t numStatefulFFs = m_mgr.GetSystem().GetStatefulFeatureFunctions().size();
  // coverage
//  if (m_sourceCompleted != other.m_sourceCompleted) {
//	return false;
// }

  // states
  for (size_t i = 0; i < numStatefulFFs; ++i) {
	const Moses::FFState &thisState = *m_ffStates[i];
	const Moses::FFState &otherState = *other.m_ffStates[i];
	if (thisState != otherState) {
	  return false;
	}
  }
  return true;

}

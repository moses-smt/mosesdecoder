/*
 * Hypothesis.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include "Hypothesis.h"
#include "Manager.h"
#include "StaticData.h"

Hypothesis::Hypothesis(const Manager &mgr, const Moses::WordsBitmap &bitmap, const Moses::WordsRange &range)
:m_mgr(mgr)
,m_bitmap(bitmap)
,m_range(range)
{
	util::Pool &pool = mgr.GetPool();
	size_t numStatefulFFs = mgr.GetStaticData().GetStatefulFeatureFunctions().size();
	m_ffStates = (Moses::FFState **) pool.Allocate(sizeof(Moses::FFState*) * numStatefulFFs);
}

Hypothesis::~Hypothesis() {
	// TODO Auto-generated destructor stub
}

size_t Hypothesis::hash() const
{
  size_t numStatefulFFs = m_mgr.GetStaticData().GetStatefulFeatureFunctions().size();
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
	size_t numStatefulFFs = m_mgr.GetStaticData().GetStatefulFeatureFunctions().size();
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

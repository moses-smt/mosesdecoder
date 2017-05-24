/*
 * Hypothesis.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include <stdlib.h>
#include <deque>
#include "HypothesisBase.h"
#include "System.h"
#include "Scores.h"
#include "ManagerBase.h"
#include "MemPool.h"
#include "FF/StatefulFeatureFunction.h"

using namespace std;

namespace Moses2
{

//size_t g_numHypos = 0;

HypothesisBase::HypothesisBase(MemPool &pool, const System &system)
{
  m_scores = new (pool.Allocate<Scores>()) Scores(system, pool,
      system.featureFunctions.GetNumScores());

  // FF states
  const std::vector<const StatefulFeatureFunction*> &sfffs =
    system.featureFunctions.GetStatefulFeatureFunctions();
  size_t numStatefulFFs = sfffs.size();
  m_ffStates = (FFState **) pool.Allocate(sizeof(FFState*) * numStatefulFFs);

  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
    size_t statefulInd = sfff->GetStatefulInd();
    FFState *state = sfff->BlankState(pool, system);
    m_ffStates[statefulInd] = state;
  }
}

size_t HypothesisBase::hash() const
{
  return hash(0);
}

size_t HypothesisBase::hash(size_t seed) const
{
  size_t numStatefulFFs =
    GetManager().system.featureFunctions.GetStatefulFeatureFunctions().size();

  // states
  for (size_t i = 0; i < numStatefulFFs; ++i) {
    const FFState *state = m_ffStates[i];
    size_t hash = state->hash();
    boost::hash_combine(seed, hash);
  }
  return seed;

}

bool HypothesisBase::operator==(const HypothesisBase &other) const
{
  size_t numStatefulFFs =
    GetManager().system.featureFunctions.GetStatefulFeatureFunctions().size();

  // states
  for (size_t i = 0; i < numStatefulFFs; ++i) {
    const FFState &thisState = *m_ffStates[i];
    const FFState &otherState = *other.m_ffStates[i];
    if (thisState != otherState) {
      return false;
    }
  }
  return true;

}

}


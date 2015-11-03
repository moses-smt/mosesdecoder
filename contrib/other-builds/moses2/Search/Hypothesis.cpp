/*
 * Hypothesis.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include <stdlib.h>
#include "Hypothesis.h"
#include "Manager.h"
#include "../System.h"
#include "../Scores.h"
#include "../FF/StatefulFeatureFunction.h"

using namespace std;

Hypothesis::Hypothesis(Manager &mgr,
		const TargetPhrase &tp,
		const Moses::Range &range,
		const Moses::Bitmap &bitmap)
:m_mgr(mgr)
,m_targetPhrase(tp)
,m_sourceCompleted(bitmap)
,m_range(range)
,m_prevHypo(NULL)
{
	MemPool &pool = m_mgr.GetPool();

	size_t numStatefulFFs = m_mgr.GetSystem().GetFeatureFunctions().GetStatefulFeatureFunctions().size();
	m_ffStates = (const Moses::FFState **) pool.Allocate(sizeof(Moses::FFState*) * numStatefulFFs);

	m_scores = new (pool.Allocate<Scores>()) Scores(pool, m_mgr.GetSystem().GetFeatureFunctions().GetNumScores());
}

Hypothesis::Hypothesis(const Hypothesis &prevHypo,
		const TargetPhrase &tp,
		const Moses::Range &pathRange,
		const Moses::Bitmap &bitmap)
:m_mgr(prevHypo.m_mgr)
,m_targetPhrase(tp)
,m_sourceCompleted(bitmap)
,m_range(pathRange)
,m_prevHypo(&prevHypo)
{
	MemPool &pool = m_mgr.GetPool();
	size_t numStatefulFFs = m_mgr.GetSystem().GetFeatureFunctions().GetStatefulFeatureFunctions().size();
	m_ffStates = (const Moses::FFState **) pool.Allocate(sizeof(Moses::FFState*) * numStatefulFFs);

	m_scores = new (pool.Allocate<Scores>())
			Scores(pool,
					m_mgr.GetSystem().GetFeatureFunctions().GetNumScores(),
					prevHypo.GetScores());
	m_scores->PlusEquals(m_mgr.GetSystem(), m_targetPhrase.GetScores());
}

Hypothesis::~Hypothesis() {
	// TODO Auto-generated destructor stub
}

size_t Hypothesis::hash() const
{
  size_t numStatefulFFs = m_mgr.GetSystem().GetFeatureFunctions().GetStatefulFeatureFunctions().size();
  size_t seed;

  // coverage
  seed = (size_t) &m_sourceCompleted;

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
	size_t numStatefulFFs = m_mgr.GetSystem().GetFeatureFunctions().GetStatefulFeatureFunctions().size();
  // coverage
  if (&m_sourceCompleted != &other.m_sourceCompleted) {
	return false;
 }

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

void Hypothesis::OutputToStream(std::ostream &out) const
{
  if (m_prevHypo) {
	  m_prevHypo->OutputToStream(out);
  }

  if (m_targetPhrase.GetSize()) {
	  out << (const Phrase&) m_targetPhrase;
	  out << " ";
  }
}

std::ostream& operator<<(std::ostream &out, const Hypothesis &obj)
{
	obj.OutputToStream(out);
	out << " ";
	obj.GetScores().Debug(out, obj.m_mgr.GetSystem().GetFeatureFunctions());
	return out;
}

void Hypothesis::EmptyHypothesisState(const Phrase &input)
{
	const std::vector<const StatefulFeatureFunction*>  &sfffs = m_mgr.GetSystem().GetFeatureFunctions().GetStatefulFeatureFunctions();
	  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
		  size_t statefulInd = sfff->GetStatefulInd();
		  const Moses::FFState *state = sfff->EmptyHypothesisState(m_mgr, input);
		  m_ffStates[statefulInd] = state;
	  }
}

void Hypothesis::EvaluateWhenApplied()
{
  const std::vector<const StatefulFeatureFunction*>  &sfffs = m_mgr.GetSystem().GetFeatureFunctions().GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
	  size_t statefulInd = sfff->GetStatefulInd();
	  const Moses::FFState *prevState = m_prevHypo->GetState(statefulInd);
	  assert(prevState);
	  const Moses::FFState *state = sfff->EvaluateWhenApplied(m_mgr, *this, *prevState, *m_scores);
	  m_ffStates[statefulInd] = state;
  }

  //cerr << *this << endl;
}


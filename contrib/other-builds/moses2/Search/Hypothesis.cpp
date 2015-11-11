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
#include "../Recycler.h"
#include "../FF/StatefulFeatureFunction.h"

using namespace std;

//size_t g_numHypos = 0;

Hypothesis *Hypothesis::Create(Manager &mgr)
{
//	++g_numHypos;
	Hypothesis *ret;

	Recycler<Hypothesis*> &recycler = mgr.GetHypoRecycle();
	if (recycler.empty()) {
		MemPool &pool = mgr.GetPool();
		ret = new (pool.Allocate<Hypothesis>()) Hypothesis(mgr);
	}
	else {
		ret = recycler.get();
		recycler.pop();
	}
	return ret;
}

Hypothesis::Hypothesis(Manager &mgr)
:m_mgr(mgr)
,m_currTargetWordsRange()
{
	MemPool &pool = m_mgr.GetPool();

	m_scores = new (pool.Allocate<Scores>()) Scores(pool, m_mgr.system.featureFunctions.GetNumScores());

	// FF states
	const std::vector<const StatefulFeatureFunction*> &sfffs = m_mgr.system.featureFunctions.GetStatefulFeatureFunctions();
	size_t numStatefulFFs = sfffs.size();
	m_ffStates = (Moses::FFState **) pool.Allocate(sizeof(Moses::FFState*) * numStatefulFFs);

    BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
    	size_t statefulInd = sfff->GetStatefulInd();
    	Moses::FFState *state = sfff->BlankState(mgr, mgr.GetInput());
    	m_ffStates[statefulInd] = state;
    }
}

Hypothesis::~Hypothesis() {
	// TODO Auto-generated destructor stub
}

void Hypothesis::Init(const TargetPhrase &tp,
		const Range &range,
		const Bitmap &bitmap)
{
	m_targetPhrase = &tp;
	m_sourceCompleted = &bitmap;
	m_range = &range;
	m_prevHypo = NULL;
	m_currTargetWordsRange = Range(NOT_FOUND, NOT_FOUND);
	m_estimatedScore = 0;

	size_t numScores = m_mgr.system.featureFunctions.GetNumScores();
	m_scores->Reset(numScores);
}

void Hypothesis::Init(const Hypothesis &prevHypo,
		const TargetPhrase &tp,
		const Range &pathRange,
		const Bitmap &bitmap,
		SCORE estimatedScore)
{
	m_targetPhrase = &tp;
	m_sourceCompleted = &bitmap;
	m_range = &pathRange;
	m_prevHypo = &prevHypo;
	m_currTargetWordsRange = Range(prevHypo.m_currTargetWordsRange.GetEndPos() + 1,
	                         prevHypo.m_currTargetWordsRange.GetEndPos()
	                         + tp.GetSize());
	m_estimatedScore = estimatedScore;

	size_t numScores = m_mgr.system.featureFunctions.GetNumScores();
	m_scores->Reset(numScores);
	m_scores->PlusEquals(m_mgr.system, prevHypo.GetScores());
	m_scores->PlusEquals(m_mgr.system, GetTargetPhrase().GetScores());
}

size_t Hypothesis::hash() const
{
  size_t numStatefulFFs = m_mgr.system.featureFunctions.GetStatefulFeatureFunctions().size();
  size_t seed;

  // coverage
  seed = (size_t) m_sourceCompleted;

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
	size_t numStatefulFFs = m_mgr.system.featureFunctions.GetStatefulFeatureFunctions().size();
  // coverage
  if (m_sourceCompleted != other.m_sourceCompleted) {
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

  if (GetTargetPhrase().GetSize()) {
	  const Phrase &phrase = GetTargetPhrase();
	  out << phrase << " ";
  }
}

std::ostream& operator<<(std::ostream &out, const Hypothesis &obj)
{
	obj.OutputToStream(out);
	out << " ";
	obj.GetScores().Debug(out, obj.m_mgr.system.featureFunctions);
	return out;
}

void Hypothesis::EmptyHypothesisState(const PhraseImpl &input)
{
	const std::vector<const StatefulFeatureFunction*>  &sfffs = m_mgr.system.featureFunctions.GetStatefulFeatureFunctions();
	  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
		  size_t statefulInd = sfff->GetStatefulInd();
		  Moses::FFState *state = m_ffStates[statefulInd];
		  sfff->EmptyHypothesisState(*state, m_mgr, input);
	  }
}

void Hypothesis::EvaluateWhenApplied()
{
  const std::vector<const StatefulFeatureFunction*>  &sfffs = m_mgr.system.featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
	  size_t statefulInd = sfff->GetStatefulInd();
	  const Moses::FFState *prevState = m_prevHypo->GetState(statefulInd);
	  Moses::FFState *thisState = m_ffStates[statefulInd];
	  assert(prevState);
	  sfff->EvaluateWhenApplied(m_mgr, *this, *prevState, *m_scores, *thisState);
  }

  //cerr << *this << endl;
}

/** recursive - pos is relative from start of sentence */
const Word &Hypothesis::GetWord(size_t pos) const {
  const Hypothesis *hypo = this;
  while (pos < hypo->GetCurrTargetWordsRange().GetStartPos()) {
    hypo = hypo->GetPrevHypo();
    UTIL_THROW_IF2(hypo == NULL, "Previous hypothesis should not be NULL");
  }
  return hypo->GetCurrWord(pos - hypo->GetCurrTargetWordsRange().GetStartPos());
}

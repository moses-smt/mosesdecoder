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
:mgr(mgr)
,m_currTargetWordsRange()
{
	MemPool &pool = mgr.GetPool();

	m_scores = new (pool.Allocate<Scores>()) Scores(pool, mgr.system.featureFunctions.GetNumScores());

	// FF states
	const std::vector<const StatefulFeatureFunction*> &sfffs = mgr.system.featureFunctions.GetStatefulFeatureFunctions();
	size_t numStatefulFFs = sfffs.size();
	m_ffStates = (FFState **) pool.Allocate(sizeof(FFState*) * numStatefulFFs);

    BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
    	size_t statefulInd = sfff->GetStatefulInd();
    	FFState *state = sfff->BlankState(mgr, mgr.GetInput());
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

	size_t numScores = mgr.system.featureFunctions.GetNumScores();
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

	size_t numScores = mgr.system.featureFunctions.GetNumScores();
	m_scores->Reset(numScores);
	m_scores->PlusEquals(mgr.system, prevHypo.GetScores());
	m_scores->PlusEquals(mgr.system, GetTargetPhrase().GetScores());
}

size_t Hypothesis::hash() const
{
  size_t numStatefulFFs = mgr.system.featureFunctions.GetStatefulFeatureFunctions().size();
  size_t seed;

  // coverage
  seed = (size_t) m_sourceCompleted;

  // states
  for (size_t i = 0; i < numStatefulFFs; ++i) {
	const FFState *state = m_ffStates[i];
	size_t hash = state->hash();
	boost::hash_combine(seed, hash);
  }
  return seed;

}

bool Hypothesis::operator==(const Hypothesis &other) const
{
	size_t numStatefulFFs = mgr.system.featureFunctions.GetStatefulFeatureFunctions().size();
  // coverage
  if (m_sourceCompleted != other.m_sourceCompleted) {
	return false;
 }

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
	obj.GetScores().Debug(out, obj.mgr.system.featureFunctions);
	return out;
}

void Hypothesis::EmptyHypothesisState(const PhraseImpl &input)
{
	const std::vector<const StatefulFeatureFunction*>  &sfffs = mgr.system.featureFunctions.GetStatefulFeatureFunctions();
	  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
		  size_t statefulInd = sfff->GetStatefulInd();
		  FFState *state = m_ffStates[statefulInd];
		  sfff->EmptyHypothesisState(*state, mgr, input);
	  }
}

void Hypothesis::EvaluateWhenApplied()
{
  const std::vector<const StatefulFeatureFunction*>  &sfffs = mgr.system.featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
	  EvaluateWhenApplied(*sfff);
  }

  //cerr << *this << endl;
}

void Hypothesis::EvaluateWhenApplied(const StatefulFeatureFunction &sfff)
{
	  size_t statefulInd = sfff.GetStatefulInd();
	  const FFState *prevState = m_prevHypo->GetState(statefulInd);
	  FFState *thisState = m_ffStates[statefulInd];
	  assert(prevState);
	  sfff.EvaluateWhenApplied(mgr, *this, *prevState, *m_scores, *thisState);

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

template<typename T>
void Swap(T &a, T &b) {
  T &c = a;
  a = b;
  b = c;
}

void Hypothesis::Swap(Hypothesis &other)
{
	::Swap(m_targetPhrase, other.m_targetPhrase);
	::Swap(m_sourceCompleted, other.m_sourceCompleted);
	::Swap(m_range, other.m_range);
	::Swap(m_prevHypo, other.m_prevHypo);
	::Swap(m_ffStates, other.m_ffStates);
	::Swap(m_estimatedScore, other.m_estimatedScore);
	::Swap(m_currTargetWordsRange, other.m_currTargetWordsRange);


}


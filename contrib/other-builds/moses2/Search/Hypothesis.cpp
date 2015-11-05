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

Hypothesis::Hypothesis(Manager &mgr)
:m_mgr(mgr)
,m_currTargetWordsRange()
{
	MemPool &pool = m_mgr.GetPool();

	m_scores = new (pool.Allocate<Scores>()) Scores(pool, m_mgr.GetSystem().featureFunctions.GetNumScores());

	// FF states
	const std::vector<const StatefulFeatureFunction*> &sfffs = m_mgr.GetSystem().featureFunctions.GetStatefulFeatureFunctions();
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
		const Moses::Range &range,
		const Moses::Bitmap &bitmap)
{
	m_targetPhrase = &tp;
	m_sourceCompleted = &bitmap;
	m_range = &range;
	m_prevHypo = NULL;
	m_currTargetWordsRange = Moses::Range(NOT_FOUND, NOT_FOUND);

	size_t numScores = m_mgr.GetSystem().featureFunctions.GetNumScores();
	m_scores->Reset(numScores);
}

void Hypothesis::Init(const Hypothesis &prevHypo,
		const TargetPhrase &tp,
		const Moses::Range &pathRange,
		const Moses::Bitmap &bitmap)
{
	m_targetPhrase = &tp;
	m_sourceCompleted = &bitmap;
	m_range = &pathRange;
	m_prevHypo = &prevHypo;
	m_currTargetWordsRange = Moses::Range(prevHypo.m_currTargetWordsRange.GetEndPos() + 1,
	                         prevHypo.m_currTargetWordsRange.GetEndPos()
	                         + tp.GetSize());

	size_t numScores = m_mgr.GetSystem().featureFunctions.GetNumScores();
	m_scores->Reset(numScores);
	m_scores->PlusEquals(m_mgr.GetSystem(), prevHypo.GetScores());
	m_scores->PlusEquals(m_mgr.GetSystem(), GetTargetPhrase().GetScores());
}

size_t Hypothesis::hash() const
{
  size_t numStatefulFFs = m_mgr.GetSystem().featureFunctions.GetStatefulFeatureFunctions().size();
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
	size_t numStatefulFFs = m_mgr.GetSystem().featureFunctions.GetStatefulFeatureFunctions().size();
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
	obj.GetScores().Debug(out, obj.m_mgr.GetSystem().featureFunctions);
	return out;
}

void Hypothesis::EmptyHypothesisState(const PhraseImpl &input)
{
	const std::vector<const StatefulFeatureFunction*>  &sfffs = m_mgr.GetSystem().featureFunctions.GetStatefulFeatureFunctions();
	  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
		  size_t statefulInd = sfff->GetStatefulInd();
		  Moses::FFState *state = sfff->EmptyHypothesisState(m_mgr, input);
		  m_ffStates[statefulInd] = state;
	  }
}

void Hypothesis::EvaluateWhenApplied()
{
  const std::vector<const StatefulFeatureFunction*>  &sfffs = m_mgr.GetSystem().featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
	  size_t statefulInd = sfff->GetStatefulInd();
	  const Moses::FFState *prevState = m_prevHypo->GetState(statefulInd);
	  assert(prevState);
	  Moses::FFState *state = sfff->EvaluateWhenApplied(m_mgr, *this, *prevState, *m_scores);
	  m_ffStates[statefulInd] = state;
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

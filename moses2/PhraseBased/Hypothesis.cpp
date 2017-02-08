/*
 * Hypothesis.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu hoang
 */
#include <boost/foreach.hpp>
#include <stdlib.h>
#include <deque>
#include "Hypothesis.h"
#include "Manager.h"
#include "Sentence.h"
#include "TargetPhraseImpl.h"
#include "../InputPathBase.h"
#include "../System.h"
#include "../Scores.h"
#include "../Phrase.h"
#include "../FF/StatefulFeatureFunction.h"

using namespace std;

namespace Moses2
{
Hypothesis *Hypothesis::Create(MemPool &pool, Manager &mgr)
{
//	++g_numHypos;
  Hypothesis *ret;

  Recycler<HypothesisBase*> &recycler = mgr.GetHypoRecycle();
  ret = static_cast<Hypothesis*>(recycler.Get());
  if (ret) {
    // got new hypo from recycler. Do nothing
  } else {
    ret = new (pool.Allocate<Hypothesis>()) Hypothesis(pool, mgr.system);
    //cerr << "Hypothesis=" << sizeof(Hypothesis) << " " << ret << endl;
    recycler.Keep(ret);
  }
  return ret;
}

Hypothesis::Hypothesis(MemPool &pool, const System &system) :
  HypothesisBase(pool, system), m_currTargetWordsRange()
{
}

Hypothesis::~Hypothesis()
{
  // TODO Auto-generated destructor stub
}

void Hypothesis::Init(Manager &mgr, const InputPathBase &path,
                      const TargetPhraseImpl &tp, const Bitmap &bitmap)
{
  m_mgr = &mgr;
  m_targetPhrase = &tp;
  m_sourceCompleted = &bitmap;
  m_path = &path;
  m_prevHypo = NULL;

  m_currTargetWordsRange.SetStartPos(NOT_FOUND);
  m_currTargetWordsRange.SetEndPos(NOT_FOUND);

  m_estimatedScore = 0;
  m_scores->Reset(mgr.system);
}

void Hypothesis::Init(Manager &mgr, const Hypothesis &prevHypo,
                      const InputPathBase &path, const TargetPhraseImpl &tp, const Bitmap &bitmap,
                      SCORE estimatedScore)
{
  m_mgr = &mgr;
  m_targetPhrase = &tp;
  m_sourceCompleted = &bitmap;
  m_path = &path;
  m_prevHypo = &prevHypo;

  m_currTargetWordsRange.SetStartPos(
    prevHypo.m_currTargetWordsRange.GetEndPos() + 1);
  m_currTargetWordsRange.SetEndPos(
    prevHypo.m_currTargetWordsRange.GetEndPos() + tp.GetSize());

  m_estimatedScore = estimatedScore;

  m_scores->Reset(mgr.system);
  m_scores->PlusEquals(mgr.system, prevHypo.GetScores());
  m_scores->PlusEquals(mgr.system, GetTargetPhrase().GetScores());
}

size_t Hypothesis::hash() const
{
  // coverage
  size_t seed = (size_t) m_sourceCompleted;

  seed = HypothesisBase::hash(seed);
  return seed;
}

bool Hypothesis::operator==(const Hypothesis &other) const
{
  // coverage
  if (m_sourceCompleted != other.m_sourceCompleted) {
    return false;
  }

  bool ret = HypothesisBase::operator ==(other);
  return ret;
}

std::string Hypothesis::Debug(const System &system) const
{
  stringstream out;

  // coverage
  out << GetBitmap() << " " << GetInputPath().range << " ";

  // states
  const std::vector<const StatefulFeatureFunction*> &sfffs =
    GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
  size_t numStatefulFFs = sfffs.size();
  for (size_t i = 0; i < numStatefulFFs; ++i) {
    const FFState &state = *GetState(i);
    out << "(" << state << ") ";
  }

  // string
  //Debug(out, m_mgr->system);
  out << " ";
  out << "fc=" << GetFutureScore() << " ";
  out << GetScores().Debug(GetManager().system);

  return out.str();
}

void Hypothesis::OutputToStream(std::ostream &out) const
{
  if (m_prevHypo) {
    m_prevHypo->OutputToStream(out);
  }
  //cerr << "range=" << GetInputPath().range << endl;

  const TargetPhrase<Moses2::Word> &tp = GetTargetPhrase();
  if (tp.GetSize()) {
    const SubPhrase<Moses2::Word> &subPhrase = static_cast<const InputPath&>(GetInputPath()).subPhrase;
    //cerr << "tp=" << tp.Debug(m_mgr->system) << endl;
    //cerr << "subPhrase=" << subPhrase.Debug(m_mgr->system) << endl;

    tp.OutputToStream(m_mgr->system, subPhrase, out);
  }

  if (m_path->range.GetStartPos() != NOT_FOUND) {
    if (m_mgr->system.options.output.ReportSegmentation == 1) {
      // just report phrase segmentation
      out << "|" << m_path->range.GetStartPos() << "-" << m_path->range.GetEndPos() << "| ";
    } else if (m_mgr->system.options.output.ReportSegmentation == 2) {
      // more detailed info about every segment
      out << "|";

      // phrase segmentation
      out << m_path->range.GetStartPos() << "-" << m_path->range.GetEndPos() << ",";

      // score breakdown
      m_scores->OutputBreakdown(out, m_mgr->system);

      out << "| ";
    }
  }
}

void Hypothesis::EmptyHypothesisState(const InputType &input)
{
  const std::vector<const StatefulFeatureFunction*> &sfffs =
    GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
    size_t statefulInd = sfff->GetStatefulInd();
    FFState *state = m_ffStates[statefulInd];
    sfff->EmptyHypothesisState(*state, GetManager(), input, *this);
  }
}

void Hypothesis::EvaluateWhenApplied()
{
  const std::vector<const StatefulFeatureFunction*> &sfffs =
    GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
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
  sfff.EvaluateWhenApplied(GetManager(), *this, *prevState, *m_scores,
                           *thisState);

}

/** recursive - pos is relative from start of sentence */
const Word &Hypothesis::GetWord(size_t pos) const
{
  const Hypothesis *hypo = this;
  while (pos < hypo->GetCurrTargetWordsRange().GetStartPos()) {
    hypo = hypo->GetPrevHypo();
    UTIL_THROW_IF2(hypo == NULL, "Previous hypothesis should not be NULL");
  }
  return hypo->GetCurrWord(pos - hypo->GetCurrTargetWordsRange().GetStartPos());
}

void Hypothesis::Swap(Hypothesis &other)
{
  /*
   Swap(m_targetPhrase, other.m_targetPhrase);
   Swap(m_sourceCompleted, other.m_sourceCompleted);
   Swap(m_range, other.m_range);
   Swap(m_prevHypo, other.m_prevHypo);
   Swap(m_ffStates, other.m_ffStates);
   Swap(m_estimatedScore, other.m_estimatedScore);
   Swap(m_currTargetWordsRange, other.m_currTargetWordsRange);
   */
}

}


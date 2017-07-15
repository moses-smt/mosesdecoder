/*
 * PhraseLR.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "PhraseBasedReorderingState.h"
#include "LexicalReordering.h"
#include "../../PhraseBased/Hypothesis.h"
#include "../../InputPathBase.h"
#include "../../PhraseBased/Manager.h"

using namespace std;

namespace Moses2
{

PhraseBasedReorderingState::PhraseBasedReorderingState(const LRModel &config,
    LRModel::Direction dir, size_t offset) :
  LRState(config, dir, offset)
{
  // uninitialised
  prevPath = NULL;
  prevTP = NULL;
}

void PhraseBasedReorderingState::Init(const LRState *prev,
                                      const TargetPhrase<Moses2::Word> &topt, const InputPathBase &path, bool first,
                                      const Bitmap *coverage)
{
  prevTP = &topt;
  prevPath = &path;
  m_first = first;
}

size_t PhraseBasedReorderingState::hash() const
{
  size_t ret;
  ret = (size_t) &prevPath->range;
  boost::hash_combine(ret, m_direction);

  return ret;
}

bool PhraseBasedReorderingState::operator==(const FFState& o) const
{
  if (&o == this) return true;

  const PhraseBasedReorderingState &other =
    static_cast<const PhraseBasedReorderingState&>(o);
  if (&prevPath->range == &other.prevPath->range) {
    if (m_direction == LRModel::Forward) {
      int compareScore = ComparePrevScores(other.prevTP);
      return compareScore == 0;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

void PhraseBasedReorderingState::Expand(const ManagerBase &mgr,
                                        const LexicalReordering &ff, const Hypothesis &hypo, size_t phraseTableInd,
                                        Scores &scores, FFState &state) const
{
  if ((m_direction != LRModel::Forward) || !m_first) {
    LRModel const& lrmodel = m_configuration;
    Range const &cur = hypo.GetInputPath().range;
    LRModel::ReorderingType reoType = (
                                        m_first ?
                                        lrmodel.GetOrientation(cur) :
                                        lrmodel.GetOrientation(prevPath->range, cur));
    CopyScores(mgr.system, scores, hypo.GetTargetPhrase(), reoType);
  }

  PhraseBasedReorderingState &stateCast =
    static_cast<PhraseBasedReorderingState&>(state);
  stateCast.Init(this, hypo.GetTargetPhrase(), hypo.GetInputPath(), false,
                 NULL);
}

} /* namespace Moses2 */

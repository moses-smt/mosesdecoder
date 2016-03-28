/*
 * HReorderingForwardState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "HReorderingForwardState.h"
#include "../../InputPathBase.h"
#include "../../PhraseBased/Manager.h"
#include "../../PhraseBased/Hypothesis.h"

namespace Moses2 {

HReorderingForwardState::HReorderingForwardState(
    const LRModel &config,
    size_t offset)
: LRState(config, LRModel::Forward, offset)
, m_first(true)
{
  prevPath = NULL;
  m_coverage = NULL;
}

HReorderingForwardState::~HReorderingForwardState() {
	// TODO Auto-generated destructor stub
}

void HReorderingForwardState::Init(
    const LRState *prev,
        const TargetPhrase &topt,
    const InputPathBase &path,
    bool first,
    const Bitmap *coverage)
{
  prevTP = &topt;
  prevPath = &path;
  m_first = first;
  m_coverage = coverage;
}

size_t HReorderingForwardState::hash() const
{
  size_t ret;
  ret = (size_t) &prevPath->range;
  boost::hash_combine(ret, m_direction);

  return ret;
}

bool HReorderingForwardState::operator==(const FFState& o) const
{
  if (&o == this) return true;

  const HReorderingForwardState &other = static_cast<const HReorderingForwardState&>(o);
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

std::string HReorderingForwardState::ToString() const
{
  return "HReorderingForwardState";
}

void HReorderingForwardState::Expand(const ManagerBase &mgr,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const
{
  const Range &cur = hypo.GetInputPath().range;
  // keep track of the current coverage ourselves so we don't need the hypothesis
  Manager &mgrCast = const_cast<Manager&>(static_cast<const Manager&>(mgr));
  Bitmaps &bms = mgrCast.GetBitmaps();
  const Bitmap &cov = bms.GetBitmap(*m_coverage, cur);

  if (!m_first) {
    LRModel::ReorderingType reoType;
    reoType = m_configuration.GetOrientation(prevPath->range, cur, cov);
    CopyScores(mgr.system, scores, hypo.GetTargetPhrase(), reoType);
  }

  HReorderingForwardState &stateCast = static_cast<HReorderingForwardState&>(state);
  stateCast.Init(this, hypo.GetTargetPhrase(), hypo.GetInputPath(), false, &cov);
}

} /* namespace Moses2 */

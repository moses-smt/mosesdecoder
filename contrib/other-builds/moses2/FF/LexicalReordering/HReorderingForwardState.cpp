/*
 * HReorderingForwardState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "HReorderingForwardState.h"
#include "../../InputPathBase.h"

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

void HReorderingForwardState::Expand(const System &system,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const
{

}

} /* namespace Moses2 */

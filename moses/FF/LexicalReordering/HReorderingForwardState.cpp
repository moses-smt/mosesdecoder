#include "HReorderingForwardState.h"

namespace Moses
{

///////////////////////////
//HReorderingForwardState

HReorderingForwardState::
HReorderingForwardState(const LRModel &config,
                        size_t size, size_t offset)
  : LRState(config, LRModel::Forward, offset)
  , m_first(true)
  , m_prevRange(NOT_FOUND,NOT_FOUND)
  , m_coverage(size)
{ }

HReorderingForwardState::
HReorderingForwardState(const HReorderingForwardState *prev,
                        const TranslationOption &topt)
  : LRState(prev, topt)
  , m_first(false)
  , m_prevRange(topt.GetSourceWordsRange())
  , m_coverage(prev->m_coverage, topt.GetSourceWordsRange())
{
}

size_t HReorderingForwardState::hash() const
{
  size_t ret;
  ret = hash_value(m_prevRange);
  return ret;
}

bool HReorderingForwardState::operator==(const FFState& o) const
{
  if (&o == this) return true;

  HReorderingForwardState const& other
  = static_cast<HReorderingForwardState const&>(o);

  int compareScores = ((m_prevRange == other.m_prevRange)
                       ? ComparePrevScores(other.m_prevOption)
                       : (m_prevRange < other.m_prevRange) ? -1 : 1);
  return compareScores == 0;
}

// For compatibility with the phrase-based reordering model, scoring is one
// step delayed.
// The forward model takes determines orientations heuristically as follows:
//  mono:   if the next phrase comes after the conditioning phrase and
//          - there is a gap to the right of the conditioning phrase, or
//          - the next phrase immediately follows it
//  swap:   if the next phrase goes before the conditioning phrase and
//          - there is a gap to the left of the conditioning phrase, or
//          - the next phrase immediately precedes it
//  dright: if the next phrase follows the conditioning phrase and other
//          stuff comes in between
//  dleft:  if the next phrase precedes the conditioning phrase and other
//          stuff comes in between

LRState*
HReorderingForwardState::
Expand(TranslationOption const& topt, InputType const& input,
       ScoreComponentCollection* scores) const
{
  const Range cur = topt.GetSourceWordsRange();
  // keep track of the current coverage ourselves so we don't need the hypothesis
  Bitmap cov(m_coverage, cur);
  if (!m_first) {
    LRModel::ReorderingType reoType;
    reoType = m_configuration.GetOrientation(m_prevRange,cur,cov);
    CopyScores(scores, topt, input, reoType);
  }
  return new HReorderingForwardState(this, topt);
}

}

#include "PhraseBasedReorderingState.h"

namespace Moses
{
// ===========================================================================
// PHRASE BASED REORDERING STATE
// ===========================================================================
bool PhraseBasedReorderingState::m_useFirstBackwardScore = true;

PhraseBasedReorderingState::
PhraseBasedReorderingState(const PhraseBasedReorderingState *prev,
                           const TranslationOption &topt)
  : LRState(prev, topt)
  , m_prevRange(topt.GetSourceWordsRange())
  , m_first(false)
{ }


PhraseBasedReorderingState::
PhraseBasedReorderingState(const LRModel &config,
                           LRModel::Direction dir, size_t offset)
  : LRState(config, dir, offset)
  , m_prevRange(NOT_FOUND,NOT_FOUND)
  , m_first(true)
{ }


size_t PhraseBasedReorderingState::hash() const
{
  size_t ret;
  ret = hash_value(m_prevRange);
  boost::hash_combine(ret, m_direction);

  return ret;
}

bool PhraseBasedReorderingState::operator==(const FFState& o) const
{
  if (&o == this) return true;

  const PhraseBasedReorderingState &other = static_cast<const PhraseBasedReorderingState&>(o);
  if (m_prevRange == other.m_prevRange) {
    if (m_direction == LRModel::Forward) {
      int compareScore = ComparePrevScores(other.m_prevOption);
      return compareScore == 0;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

LRState*
PhraseBasedReorderingState::
Expand(const TranslationOption& topt, const InputType& input,
       ScoreComponentCollection* scores) const
{
  // const LRModel::ModelType modelType = m_configuration.GetModelType();

  if ((m_direction != LRModel::Forward && m_useFirstBackwardScore) || !m_first) {
    LRModel const& lrmodel = m_configuration;
    Range const cur = topt.GetSourceWordsRange();
    LRModel::ReorderingType reoType = (m_first ? lrmodel.GetOrientation(cur)
                                       : lrmodel.GetOrientation(m_prevRange,cur));
    CopyScores(scores, topt, input, reoType);
  }
  return new PhraseBasedReorderingState(this, topt);
}

}


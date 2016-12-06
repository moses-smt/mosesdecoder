#include "BidirectionalReorderingState.h"

namespace Moses
{

///////////////////////////
//BidirectionalReorderingState

size_t BidirectionalReorderingState::hash() const
{
  size_t ret = m_backward->hash();
  boost::hash_combine(ret, m_forward->hash());
  return ret;
}

bool BidirectionalReorderingState::operator==(const FFState& o) const
{
  if (&o == this) return true;

  BidirectionalReorderingState const &other
  = static_cast<BidirectionalReorderingState const&>(o);

  bool ret = (*m_backward == *other.m_backward) && (*m_forward == *other.m_forward);
  return ret;
}

LRState*
BidirectionalReorderingState::
Expand(const TranslationOption& topt, const InputType& input,
       ScoreComponentCollection* scores) const
{
  LRState *newbwd = m_backward->Expand(topt,input, scores);
  LRState *newfwd = m_forward->Expand(topt, input, scores);
  return new BidirectionalReorderingState(m_configuration, newbwd, newfwd, m_offset);
}

}


#include "HReorderingBackwardState.h"

namespace Moses
{

///////////////////////////
//HierarchicalReorderingBackwardState

HReorderingBackwardState::
HReorderingBackwardState(const HReorderingBackwardState *prev,
                         const TranslationOption &topt,
                         ReorderingStack reoStack)
  : LRState(prev, topt),  m_reoStack(reoStack)
{ }

HReorderingBackwardState::
HReorderingBackwardState(const LRModel &config, size_t offset)
  : LRState(config, LRModel::Backward, offset)
{ }

size_t HReorderingBackwardState::hash() const
{
  size_t ret = m_reoStack.hash();
  return ret;
}

bool HReorderingBackwardState::operator==(const FFState& o) const
{
  const HReorderingBackwardState& other
  = static_cast<const HReorderingBackwardState&>(o);
  bool ret = m_reoStack == other.m_reoStack;
  return ret;
}

LRState*
HReorderingBackwardState::
Expand(const TranslationOption& topt, const InputType& input,
       ScoreComponentCollection*  scores) const
{
  HReorderingBackwardState* nextState;
  nextState = new HReorderingBackwardState(this, topt, m_reoStack);
  Range swrange = topt.GetSourceWordsRange();
  int reoDistance = nextState->m_reoStack.ShiftReduce(swrange);
  ReorderingType reoType = m_configuration.GetOrientation(reoDistance);
  CopyScores(scores, topt, input, reoType);
  return nextState;
}

}


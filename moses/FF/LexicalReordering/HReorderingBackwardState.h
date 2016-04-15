#pragma once
#include "LRState.h"
#include "ReorderingStack.h"

namespace Moses
{

//! State for a hierarchical reordering model (see Galley and Manning, A
//! Simple and Effective Hierarchical Phrase Reordering Model, EMNLP 2008)
//! backward state (conditioned on the previous phrase)
class HReorderingBackwardState : public LRState
{
private:
  ReorderingStack m_reoStack;
public:
  HReorderingBackwardState(const LRModel &config, size_t offset);
  HReorderingBackwardState(const HReorderingBackwardState *prev,
                           const TranslationOption &topt,
                           ReorderingStack reoStack);
  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual LRState* Expand(const TranslationOption& hypo, const InputType& input,
                          ScoreComponentCollection*  scores) const;

private:
  ReorderingType GetOrientationTypeMSD(int reoDistance) const;
  ReorderingType GetOrientationTypeMSLR(int reoDistance) const;
  ReorderingType GetOrientationTypeMonotonic(int reoDistance) const;
  ReorderingType GetOrientationTypeLeftRight(int reoDistance) const;
};

}

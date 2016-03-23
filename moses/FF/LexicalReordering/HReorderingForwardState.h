#pragma once

#include "LRState.h"
#include "moses/Range.h"
#include "moses/Bitmap.h"

namespace Moses
{

//!forward state (conditioned on the next phrase)
class HReorderingForwardState : public LRState
{
private:
  bool m_first;
  Range m_prevRange;
  Bitmap m_coverage;

public:
  HReorderingForwardState(const LRModel &config, size_t sentenceLength,
                          size_t offset);
  HReorderingForwardState(const HReorderingForwardState *prev,
                          const TranslationOption &topt);

  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual LRState* Expand(const TranslationOption& hypo,
                          const InputType& input,
                          ScoreComponentCollection* scores) const;
};

}


#pragma once
#include "LRState.h"

namespace Moses
{

class BidirectionalReorderingState
  : public LRState
{
private:
  const LRState *m_backward;
  const LRState *m_forward;
public:
  BidirectionalReorderingState(const LRModel &config,
                               const LRState *bw,
                               const LRState *fw, size_t offset)
    : LRState(config,
              LRModel::Bidirectional,
              offset)
    , m_backward(bw)
    , m_forward(fw)
  { }

  ~BidirectionalReorderingState() {
    delete m_backward;
    delete m_forward;
  }

  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  LRState*
  Expand(const TranslationOption& topt, const InputType& input,
         ScoreComponentCollection*  scores) const;
};

}


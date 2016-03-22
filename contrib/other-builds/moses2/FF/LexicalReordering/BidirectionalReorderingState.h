/*
 * BidirectionalReorderingState.h
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#pragma once
#include "PhraseBasedReorderingState.h"

namespace Moses2 {

class BidirectionalReorderingState: public LRState
{
public:
  BidirectionalReorderingState();
  virtual ~BidirectionalReorderingState();

  size_t hash() const;
  virtual bool operator==(const FFState& other) const;

protected:
  const LRState *m_backward;
  const LRState *m_forward;

};

} /* namespace Moses2 */


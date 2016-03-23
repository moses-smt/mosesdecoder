/*
 * BidirectionalReorderingState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#include <boost/functional/hash_fwd.hpp>
#include "BidirectionalReorderingState.h"
#include "HReorderingBackwardState.h"
#include "HReorderingForwardState.h"

namespace Moses2 {

BidirectionalReorderingState::BidirectionalReorderingState()
{
	HReorderingBackwardState *backward = new HReorderingBackwardState();
	m_backward = backward;

	HReorderingForwardState *forward = new HReorderingForwardState();
	m_forward = forward;
}

BidirectionalReorderingState::~BidirectionalReorderingState() {
	// TODO Auto-generated destructor stub
}

size_t BidirectionalReorderingState::hash() const
{
  size_t ret = m_backward->hash();
  boost::hash_combine(ret, m_forward->hash());
  return ret;
}

bool BidirectionalReorderingState::operator==(const FFState& o) const
{
  if (&o == this) return 0;

  BidirectionalReorderingState const &other
  = static_cast<BidirectionalReorderingState const&>(o);

  bool ret = (*m_backward == *other.m_backward) && (*m_forward == *other.m_forward);
  return ret;
}

void BidirectionalReorderingState::Expand(const System &system,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const
{

}

} /* namespace Moses2 */

/*
 * BidirectionalReorderingState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#include <boost/functional/hash_fwd.hpp>
#include "BidirectionalReorderingState.h"

namespace Moses2 {

BidirectionalReorderingState::BidirectionalReorderingState() {
	// TODO Auto-generated constructor stub

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

} /* namespace Moses2 */

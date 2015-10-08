#pragma once

#include "moses/FF/FFState.h"

namespace Moses
{

struct PointerState : public FFState {
  const void* lmstate;
  PointerState(const void* lms) {
    lmstate = lms;
  }
  int Compare(const FFState& o) const {
    const PointerState& other = static_cast<const PointerState&>(o);
    if (other.lmstate > lmstate) return 1;
    else if (other.lmstate < lmstate) return -1;
    return 0;
  }

  virtual size_t hash() const {
	  return (size_t) lmstate;
  }

};

} // namespace

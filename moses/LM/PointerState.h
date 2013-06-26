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
};

} // namespace

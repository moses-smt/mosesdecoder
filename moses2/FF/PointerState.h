#pragma once

#include <sstream>
#include "FFState.h"

namespace Moses2
{

struct PointerState: public FFState {
  const void* lmstate;

  explicit PointerState() {
    // uninitialised
  }

  PointerState(const void* lms) {
    lmstate = lms;
  }
  virtual size_t hash() const {
    return (size_t) lmstate;
  }
  virtual bool operator==(const FFState& other) const {
    const PointerState& o = static_cast<const PointerState&>(other);
    return lmstate == o.lmstate;
  }

  virtual std::string ToString() const {
    std::stringstream sb;
    sb << lmstate;
    return sb.str();
  }

};

}


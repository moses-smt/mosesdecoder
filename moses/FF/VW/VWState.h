#pragma once

#include <ostream>

#include "moses/FF/FFState.h"
#include "moses/Phrase.h"
#include "moses/Hypothesis.h"

namespace Moses {

/**
 * VW state, used in decoding (when target context is enabled).
 */
struct VWState : public FFState {
  virtual size_t hash() const;
  virtual bool operator==(const FFState& o) const;

  // shift words in our state, add words from current hypothesis
  static VWState *UpdateState(const FFState *prevState, const Hypothesis &curHypo);

  Phrase m_phrase;
};

// how to print a VW state
std::ostream &operator<<(std::ostream &out, const VWState &state);

}

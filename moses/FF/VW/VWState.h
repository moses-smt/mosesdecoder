#pragma once

#include <ostream>

#include "moses/FF/FFState.h"
#include "moses/Phrase.h"
#include "moses/Hypothesis.h"

namespace Moses
{

/**
 * VW state, used in decoding (when target context is enabled).
 */
class VWState : public FFState
{
public:
  // empty state, used only when VWState is ignored
  VWState();

  // used for construction of the initial VW state
  VWState(const Phrase &phrase);

  // continue from previous VW state with a new hypothesis
  VWState(const VWState &prevState, const Hypothesis &curHypo);

  virtual bool operator==(const FFState& o) const;

  inline virtual size_t hash() const {
    return m_hash;
  }

  inline const Phrase &GetPhrase() const {
    return m_phrase;
  }

  inline size_t GetSpanStart() const {
    return m_spanStart;
  }

  inline size_t GetSpanEnd() const {
    return m_spanEnd;
  }

private:
  void ComputeHash();

  Phrase m_phrase;
  size_t m_spanStart, m_spanEnd;
  size_t m_hash;
};

// how to print a VW state
std::ostream &operator<<(std::ostream &out, const VWState &state);

}

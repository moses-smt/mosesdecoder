#include "VWState.h"

#include "moses/FF/FFState.h"
#include "moses/Phrase.h"
#include "moses/Hypothesis.h"
#include "moses/Util.h"
#include "moses/TypeDef.h"
#include "moses/StaticData.h"

namespace Moses {

size_t VWState::hash() const {
  return hash_value(m_phrase);
}

bool VWState::operator==(const FFState& o) const {
  const VWState &other = static_cast<const VWState &>(o);
  return m_phrase == other.m_phrase;
}

VWState *VWState::UpdateState(const FFState *prevState, const Hypothesis &curHypo) {
  const VWState *prevVWState = static_cast<const VWState *>(prevState);

  VERBOSE(3, "VW :: updating state\n>> previous state: " << *prevVWState << "\n");

  // copy phrase from previous state
  Phrase phrase = prevVWState->m_phrase;
  size_t contextSize = phrase.GetSize(); // identical to VWFeatureBase::GetMaximumContextSize()
  
  // add words from current hypothesis
  phrase.Append(curHypo.GetCurrTargetPhrase());

  VERBOSE(3, ">> current hypo: " << curHypo.GetCurrTargetPhrase() << "\n");

  // get a slice of appropriate length
  Range range(phrase.GetSize() - contextSize, phrase.GetSize() - 1);
  phrase = phrase.GetSubString(range);

  // build the new state
  VWState *out = new VWState();
  out->m_phrase = phrase;

  VERBOSE(3, ">> updated state: " << *out << "\n");

  return out;
}

std::ostream &operator<<(std::ostream &out, const VWState &state) {
  out << state.m_phrase;
  return out;
}

}

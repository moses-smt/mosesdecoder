#include "VWState.h"

#include "moses/FF/FFState.h"
#include "moses/Phrase.h"
#include "moses/Hypothesis.h"
#include "moses/Util.h"
#include "moses/TypeDef.h"
#include "moses/StaticData.h"
#include "moses/TranslationOption.h"
#include <boost/functional/hash.hpp>

namespace Moses
{

VWState::VWState() : m_spanStart(0), m_spanEnd(0)
{
  ComputeHash();
}

VWState::VWState(const Phrase &phrase)
  : m_phrase(phrase), m_spanStart(0), m_spanEnd(0)
{
  ComputeHash();
}

VWState::VWState(const VWState &prevState, const Hypothesis &curHypo)
{
  VERBOSE(3, "VW :: updating state\n>> previous state: " << prevState << "\n");

  // copy phrase from previous state
  Phrase phrase = prevState.GetPhrase();
  size_t contextSize = phrase.GetSize(); // identical to VWFeatureBase::GetMaximumContextSize()

  // add words from current hypothesis
  phrase.Append(curHypo.GetCurrTargetPhrase());

  VERBOSE(3, ">> current hypo: " << curHypo.GetCurrTargetPhrase() << "\n");

  // get a slice of appropriate length
  Range range(phrase.GetSize() - contextSize, phrase.GetSize() - 1);
  m_phrase = phrase.GetSubString(range);

  // set current span start/end
  m_spanStart = curHypo.GetTranslationOption().GetStartPos();
  m_spanEnd   = curHypo.GetTranslationOption().GetEndPos();

  // compute our hash
  ComputeHash();

  VERBOSE(3, ">> updated state: " << *this << "\n");
}

bool VWState::operator==(const FFState& o) const
{
  const VWState &other = static_cast<const VWState &>(o);

  return m_phrase == other.GetPhrase()
         && m_spanStart == other.GetSpanStart()
         && m_spanEnd == other.GetSpanEnd();
}

void VWState::ComputeHash()
{
  m_hash = 0;

  boost::hash_combine(m_hash, m_phrase);
  boost::hash_combine(m_hash, m_spanStart);
  boost::hash_combine(m_hash, m_spanEnd);
}

std::ostream &operator<<(std::ostream &out, const VWState &state)
{
  out << state.GetPhrase() << "::" << state.GetSpanStart() << "-" << state.GetSpanEnd();
  return out;
}

}

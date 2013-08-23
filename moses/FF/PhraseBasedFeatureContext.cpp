#include "PhraseBasedFeatureContext.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/TranslationOption.h"

namespace Moses
{
PhraseBasedFeatureContext::PhraseBasedFeatureContext(const Hypothesis* hypothesis) :
  m_hypothesis(hypothesis),
  m_translationOption(m_hypothesis->GetTranslationOption()),
  m_source(m_hypothesis->GetManager().GetSource()) {}


}

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

PhraseBasedFeatureContext::PhraseBasedFeatureContext
      (const TranslationOption& translationOption, const InputType& source) :
  m_hypothesis(NULL),
  m_translationOption(translationOption),
  m_source(source)
{}

const TargetPhrase& PhraseBasedFeatureContext::GetTargetPhrase() const
{
  return m_translationOption.GetTargetPhrase();
}

const WordsBitmap& PhraseBasedFeatureContext::GetWordsBitmap() const
{
  if (!m_hypothesis) {
    throw std::logic_error("Coverage vector not available during pre-calculation");
  }
  return m_hypothesis->GetWordsBitmap();
}

}

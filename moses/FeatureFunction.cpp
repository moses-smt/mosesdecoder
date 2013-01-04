#include <stdexcept>

#include "util/check.hh"

#include "ChartHypothesis.h"
#include "ChartManager.h"
#include "FeatureFunction.h"
#include "Hypothesis.h"
#include "Manager.h"
#include "TranslationOption.h"


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
  m_source(source) {}

const TranslationOption& PhraseBasedFeatureContext::GetTranslationOption() const
{
  return m_translationOption;
}

const InputType& PhraseBasedFeatureContext::GetSource() const 
{
  return m_source;
}

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


ChartBasedFeatureContext::ChartBasedFeatureContext
                        (const ChartHypothesis* hypothesis):
  m_hypothesis(hypothesis),
  m_targetPhrase(hypothesis->GetCurrTargetPhrase()),
  m_source(hypothesis->GetManager().GetSource()) {}

ChartBasedFeatureContext::ChartBasedFeatureContext(
                         const TargetPhrase& targetPhrase,
                         const InputType& source):
  m_hypothesis(NULL),
  m_targetPhrase(targetPhrase),
  m_source(source) {}

const InputType& ChartBasedFeatureContext::GetSource() const
{
  return m_source;
}

const TargetPhrase& ChartBasedFeatureContext::GetTargetPhrase() const
{
  return m_targetPhrase;
}



FeatureFunction::~FeatureFunction() {}

bool StatelessFeatureFunction::IsStateless() const
{
  return true;
}

bool StatelessFeatureFunction::ComputeValueInTranslationOption() const
{
  return false;
}

bool StatefulFeatureFunction::IsStateless() const
{
  return false;
}

}


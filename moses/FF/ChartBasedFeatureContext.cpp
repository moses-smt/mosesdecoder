#include "ChartBasedFeatureContext.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"

namespace Moses
{
ChartBasedFeatureContext::ChartBasedFeatureContext
(const ChartHypothesis* hypothesis):
  m_hypothesis(hypothesis),
  m_targetPhrase(hypothesis->GetCurrTargetPhrase()),
  m_source(hypothesis->GetManager().GetSource())
{}

ChartBasedFeatureContext::ChartBasedFeatureContext(
  const TargetPhrase& targetPhrase,
  const InputType& source):
  m_hypothesis(NULL),
  m_targetPhrase(targetPhrase),
  m_source(source)
{}


}


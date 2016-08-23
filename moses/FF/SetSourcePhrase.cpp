#include "SetSourcePhrase.h"
#include "moses/TargetPhrase.h"

namespace Moses
{
SetSourcePhrase::SetSourcePhrase(const std::string &line)
  :StatelessFeatureFunction(0, line)
{
  m_tuneable = false;
  ReadParameters();
}

void SetSourcePhrase::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedScores) const
{
  targetPhrase.SetRuleSource(source);
}

}

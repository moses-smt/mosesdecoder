
#include "PhrasePenalty.h"
#include "moses/ScoreComponentCollection.h"

namespace Moses
{
PhrasePenalty::PhrasePenalty(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  ReadParameters();
}

void PhrasePenalty::Evaluate(const Phrase &source
                             , const TargetPhrase &targetPhrase
                             , ScoreComponentCollection &scoreBreakdown
                             , ScoreComponentCollection &estimatedFutureScore) const
{
  scoreBreakdown.Assign(this, 1.0f);
}

} // namespace


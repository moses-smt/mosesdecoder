#include "PhrasePenaltyProducer.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
PhrasePenaltyProducer::PhrasePenaltyProducer(const std::string &line)
  : StatelessFeatureFunction("PhrasePenalty",1, line)
{
  ReadParameters();
}

void PhrasePenaltyProducer::Evaluate(const Phrase &source
                             , const TargetPhrase &targetPhrase
                             , ScoreComponentCollection &scoreBreakdown
                             , ScoreComponentCollection &estimatedFutureScore) const
{
  scoreBreakdown.Assign(this, - 1.0f);
}

}


#include "WordPenaltyProducer.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"

namespace Moses
{
void WordPenaltyProducer::Evaluate(const TargetPhrase &targetPhrase
                    , ScoreComponentCollection &scoreBreakdown
                    , ScoreComponentCollection &estimatedFutureScore) const
{
  float score = - (float) targetPhrase.GetNumTerminals();
  scoreBreakdown.Assign(this, score);
}

}


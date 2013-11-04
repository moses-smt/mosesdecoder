#include "WordPenaltyProducer.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
WordPenaltyProducer::WordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  ReadParameters();
}

void WordPenaltyProducer::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  float score = - (float) targetPhrase.GetNumTerminals();
  scoreBreakdown.Assign(this, score);
}

}


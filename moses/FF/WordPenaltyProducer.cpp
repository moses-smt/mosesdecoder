#include "WordPenaltyProducer.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
WordPenaltyProducer *WordPenaltyProducer::s_instance = NULL;

WordPenaltyProducer::WordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  ReadParameters();

  UTIL_THROW_IF2(s_instance, "Can only have 1 word penalty feature");
  s_instance = this;
}

void WordPenaltyProducer::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  float score = - (float) targetPhrase.GetNumTerminals();
  scoreBreakdown.Assign(this, score);
}

}


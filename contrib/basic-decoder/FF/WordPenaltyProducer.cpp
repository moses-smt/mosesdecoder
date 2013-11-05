#include "WordPenaltyProducer.h"
#include "TargetPhrase.h"

WordPenaltyProducer::WordPenaltyProducer(const std::string &line)
  :StatelessFeatureFunction(line)
{
  ReadParameters();
}

void WordPenaltyProducer::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , Scores &scores
                                   , Scores &estimatedFutureScore) const
{
  SCORE numWords = - (SCORE) targetPhrase.GetSize();
  scores.Add(*this, numWords);
}

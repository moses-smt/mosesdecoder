#include "WordPenaltyProducer.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
WordPenaltyProducer::WordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction("WordPenalty",1, line)
{
  ReadParameters();
}

void WordPenaltyProducer::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  std::cerr << "WordPenaltyProducer::Evaluate(const Phrase &source, ....) START" << std::endl;
  float score = - (float) targetPhrase.GetNumTerminals();
  scoreBreakdown.Assign(this, score);
  std::cerr << "WordPenaltyProducer::Evaluate(const Phrase &source, ....) END" << std::endl;
}

}


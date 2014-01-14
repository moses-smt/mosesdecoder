#include "PhrasePenaltyProducer.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
PhrasePenalty::PhrasePenalty(const std::string &line)
  : StatelessFeatureFunction("PhrasePenalty",1, line)
{
  ReadParameters();
}

void PhrasePenalty::Evaluate(const Phrase &source
                                     , const TargetPhrase &targetPhrase
                                     , ScoreComponentCollection &scoreBreakdown
                                     , ScoreComponentCollection &estimatedFutureScore) const
{
  std::cerr << "void PhrasePenalty::Evaluate(const Phrase &source, ...) START" << std::endl;
//  scoreBreakdown.Assign(this, - 1.0f);
  scoreBreakdown.PlusEquals(this, - 1.0f);
  std::cerr << "void PhrasePenalty::Evaluate(const Phrase &source, ...) END" << std::endl;
}

/*
void PhrasePenalty::Evaluate(const InputType &source
                      , ScoreComponentCollection &scoreBreakdown) const
{
  std::cerr << "void PhrasePenalty::Evaluate(const InputType &source, ...) START" << std::endl;
//  scoreBreakdown.Assign(this, - 1.0f);
  scoreBreakdown.PlusEquals(this, - 1.0f);
  std::cerr << "void PhrasePenalty::Evaluate(const InputType &source, ...) END" << std::endl;

}
*/

}


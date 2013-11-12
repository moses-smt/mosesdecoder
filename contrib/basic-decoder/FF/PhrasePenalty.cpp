
#include "PhrasePenalty.h"
#include "Scores.h"

PhrasePenalty::PhrasePenalty(const std::string &line)
  :StatelessFeatureFunction(line)
{
  // TODO Auto-generated constructor stub

}

PhrasePenalty::~PhrasePenalty()
{
  // TODO Auto-generated destructor stub
}

void PhrasePenalty::Evaluate(const Phrase &source
                             , const TargetPhrase &targetPhrase
                             , Scores &scores
                             , Scores &estimatedFutureScore) const
{
  scores.Add(*this, 1);
}

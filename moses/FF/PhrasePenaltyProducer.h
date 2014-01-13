#pragma once

#include <string>
#include "StatelessFeatureFunction.h"
#include "util/check.hh"

namespace Moses
{
class TargetPhrase;
class ScoreComponentCollection;

class PhrasePenaltyProducer : public StatelessFeatureFunction
{
public:
  PhrasePenaltyProducer(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;

  virtual void Evaluate(const InputType &source
                        , ScoreComponentCollection &scoreBreakdown) const;

};

}

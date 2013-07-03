#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses
{

class PhrasePenalty : public StatelessFeatureFunction
{
public:
  PhrasePenalty(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;
};

} //namespace


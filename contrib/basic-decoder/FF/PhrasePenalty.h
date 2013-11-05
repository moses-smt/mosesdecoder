
#pragma once

#include "StatelessFeatureFunction.h"

class PhrasePenalty : public StatelessFeatureFunction
{
public:
  PhrasePenalty(const std::string &line);
  virtual ~PhrasePenalty();

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , Scores &scores
                , Scores &estimatedFutureScore) const;

};


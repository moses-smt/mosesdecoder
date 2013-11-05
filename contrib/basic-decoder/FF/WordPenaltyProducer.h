#pragma once

#include <string>
#include "StatelessFeatureFunction.h"


class WordPenaltyProducer : public StatelessFeatureFunction
{
public:
  WordPenaltyProducer(const std::string &line);

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , Scores &scores
                        , Scores &estimatedFutureScore) const;


};



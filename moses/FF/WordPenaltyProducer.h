#pragma once

#include <string>
#include "StatelessFeatureFunction.h"
#include "util/check.hh"

namespace Moses
{
class TargetPhrase;
class ScoreComponentCollection;

class WordPenaltyProducer : public StatelessFeatureFunction
{
public:
  WordPenaltyProducer(const std::string &line)
    : StatelessFeatureFunction("WordPenalty",1, line) {
    CHECK(m_args.size() == 0);
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;

};

}


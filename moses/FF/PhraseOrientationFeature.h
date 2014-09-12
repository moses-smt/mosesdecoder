#pragma once

#include <string>
#include "StatelessFeatureFunction.h"
#include "FFState.h"
#include "moses/Factor.h"
#include "phrase-extract/extract-ghkm/PhraseOrientation.h"

namespace Moses
{


class PhraseOrientationFeature : public StatelessFeatureFunction
{
public:
  PhraseOrientationFeature(const std::string &line);

  ~PhraseOrientationFeature() {
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;

  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {};

  void EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const
  {};

  void EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const;

};


}


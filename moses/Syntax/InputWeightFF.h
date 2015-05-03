#pragma once

#include <string>

#include "moses/FF/StatelessFeatureFunction.h"

namespace Moses
{
namespace Syntax
{

class InputWeightFF : public StatelessFeatureFunction
{
public:
  InputWeightFF(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const;

  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const;

  void EvaluateWhenApplied(const Syntax::SHyperedge &hyperedge,
                           ScoreComponentCollection* accumulator) const;


  void SetParameter(const std::string& key, const std::string& value);

  // Virtual functions from StatelessFeatureFunction that are no-ops for
  // InputWeightFF.
  void EvaluateInIsolation(const Phrase &, const TargetPhrase &,
                           ScoreComponentCollection &,
                           ScoreComponentCollection &) const {}

  void EvaluateWithSourceContext(const InputType &, const InputPath &,
                                 const TargetPhrase &, const StackVec *,
                                 ScoreComponentCollection &,
                                 ScoreComponentCollection *) const {}

  void EvaluateTranslationOptionListWithSourceContext(
    const InputType &, const TranslationOptionList &) const {}
};

}  // Syntax
}  // Moses

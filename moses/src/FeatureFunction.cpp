#include "FeatureFunction.h"
#include "Hypothesis.h"

#include <cassert>

namespace Moses {

FeatureFunction::~FeatureFunction() {}

bool StatelessFeatureFunction::IsStateless() const { return true; }
bool StatelessFeatureFunction::ComputeValueInTranslationOption() const {
  return false;
}
void StatelessFeatureFunction::Evaluate(
    const TargetPhrase& cur_hypo,
    ScoreComponentCollection* accumulator) const {
  assert(!"Please implement Evaluate or set ComputeValueInTranslationOption to true");
}

bool StatefulFeatureFunction::IsStateless() const { return false; }

FFState* OptionStatefulFeatureFunction::Evaluate(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const {
    return Evaluate(
      cur_hypo.GetTranslationOption(),
      prev_state,
      accumulator);
  }


}


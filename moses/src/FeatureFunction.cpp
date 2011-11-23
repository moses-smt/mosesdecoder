#include "FeatureFunction.h"

#include "util/check.hh"

namespace Moses
{

FeatureFunction::~FeatureFunction() {}

bool StatelessFeatureFunction::IsStateless() const
{
  return true;
}
bool StatelessFeatureFunction::ComputeValueInTranslationOption() const
{
  return false;
}
void StatelessFeatureFunction::Evaluate(
  const TargetPhrase& /* cur_hypo */,
  ScoreComponentCollection* /* accumulator */) const
{
  CHECK(!"Please implement Evaluate or set ComputeValueInTranslationOption to true");
}

bool StatefulFeatureFunction::IsStateless() const
{
  return false;
}

}


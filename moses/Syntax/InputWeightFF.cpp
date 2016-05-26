#include "InputWeightFF.h"

#include <vector>

#include "moses/ScoreComponentCollection.h"
#include "moses/Syntax/SHyperedge.h"
#include "moses/TargetPhrase.h"

namespace Moses
{
namespace Syntax
{

InputWeightFF::InputWeightFF(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  ReadParameters();
}

void InputWeightFF::EvaluateWhenApplied(const Hypothesis& hypo,
                                        ScoreComponentCollection* accumulator) const
{
  // TODO Throw exception.
  assert(false);
}

void InputWeightFF::EvaluateWhenApplied(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{
  // TODO Throw exception.
  assert(false);
}

void InputWeightFF::EvaluateWhenApplied(
  const Syntax::SHyperedge &hyperedge,
  ScoreComponentCollection* accumulator) const
{
  accumulator->PlusEquals(this, hyperedge.label.inputWeight);
}

void InputWeightFF::SetParameter(const std::string& key,
                                 const std::string& value)
{
  StatelessFeatureFunction::SetParameter(key, value);
}

}  // Syntax
}  // Moses

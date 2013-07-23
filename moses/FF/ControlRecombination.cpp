#include "ControlRecombination.h"
#include "moses/Hypothesis.h"
#include "util/exception.hh"

using namespace std;

namespace Moses {

ControlRecombination::ControlRecombination(const std::string &line)
:StatefulFeatureFunction("ControlRecombination", 0, line)
,m_type(Output)
{
}

void ControlRecombination::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "type") {
    m_type = (Type) Scan<size_t>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* ControlRecombination::Evaluate(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  ControlRecombinationState *state = new ControlRecombinationState(&cur_hypo);
  return state;
}

FFState* ControlRecombination::EvaluateChart(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  UTIL_THROW(util::Exception, "Not implemented yet");
}

const FFState* ControlRecombination::EmptyHypothesisState(const InputType &input) const
{
  ControlRecombinationState *state = new ControlRecombinationState();
}

ControlRecombinationState::ControlRecombinationState()
:m_hypo(NULL)
{
}

ControlRecombinationState::ControlRecombinationState(const Hypothesis *hypo)
:m_hypo(hypo)
{
}

int ControlRecombinationState::Compare(const FFState& other) const
{
  const ControlRecombinationState &other2 = static_cast<const ControlRecombinationState&>(other);
  const Hypothesis *otherHypo = other2.m_hypo;

  const Phrase thisOutputPhrase, otherOutputPhrase;
  m_hypo->GetOutputPhrase(thisOutputPhrase);
  otherHypo->GetOutputPhrase(otherOutputPhrase);

  int ret = thisOutputPhrase.Compare(otherOutputPhrase);
  return ret;
}

}

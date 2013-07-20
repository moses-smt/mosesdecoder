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

  const TargetPhrase *thisTargetPhrase = &m_hypo->GetCurrTargetPhrase();
  const TargetPhrase *otherTargetPhrase = &otherHypo->GetCurrTargetPhrase();

  int thisSize = thisTargetPhrase->GetSize();
  int otherPos = otherTargetPhrase->GetSize() - 1;
  for (int thisPos = thisSize - 1; thisPos >= 0; --thisPos && --otherPos) {
	if (otherPos < 0) {
	  otherHypo = otherHypo->GetPrevHypo();
	  if (otherHypo == NULL) {
	    return -1;
	  }
	  otherTargetPhrase = &otherHypo->GetCurrTargetPhrase();
	  otherPos = otherTargetPhrase->GetSize() - 1;
	}

    const Word &thisWord = thisTargetPhrase->GetWord(thisPos);
    const Word &otherWord = otherTargetPhrase->GetWord(otherPos);
    int compare = thisWord.Compare(otherWord);
    if (compare) {
      return compare;
    }
  }

  return 0;
}

}

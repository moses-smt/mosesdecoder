#include "ControlRecombination.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
ControlRecombinationState::ControlRecombinationState(const Hypothesis &hypo, const ControlRecombination &ff)
  :m_ff(ff)
{
  if (ff.GetType() == SameOutput) {
    UTIL_THROW(util::Exception, "Implemented not yet completed for phrase-based model. Need to take into account the coverage");
    hypo.GetOutputPhrase(m_outputPhrase);
  } else {
    m_hypo = &hypo;
  }
}

ControlRecombinationState::ControlRecombinationState(const ChartHypothesis &hypo, const ControlRecombination &ff)
  :m_ff(ff)
{
  if (ff.GetType() == SameOutput) {
    hypo.GetOutputPhrase(m_outputPhrase);
  } else {
    m_hypo = &hypo;
  }
}

int ControlRecombinationState::Compare(const FFState& other) const
{
  const ControlRecombinationState &otherFF = static_cast<const ControlRecombinationState&>(other);
  if (m_ff.GetType() == SameOutput) {
    bool ret = 	m_outputPhrase.Compare(otherFF.m_outputPhrase);
    return ret;
  } else {
    // compare hypo address. Won't be equal unless they're actually the same hypo
    if (m_hypo == otherFF.m_hypo)
      return 0;
    return (m_hypo < otherFF.m_hypo) ? -1 : +1;
  }
}

std::vector<float> ControlRecombination::DefaultWeights() const
{
  UTIL_THROW_IF2(m_numScoreComponents,
		  "ControlRecombination should not have any scores");
  vector<float> ret(0);
  return ret;
}

FFState* ControlRecombination::Evaluate(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  return new ControlRecombinationState(hypo, *this);
}

FFState* ControlRecombination::EvaluateChart(
  const ChartHypothesis &hypo,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new ControlRecombinationState(hypo, *this);
}

void ControlRecombination::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "type") {
    m_type = (ControlRecombinationType) Scan<int>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}


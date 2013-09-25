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
ControlRecombinationState::ControlRecombinationState(const Hypothesis &hypo)
{
	hypo.GetOutputPhrase(m_outputPhrase);
}

ControlRecombinationState::ControlRecombinationState(const ChartHypothesis &hypo)
{
	hypo.GetOutputPhrase(m_outputPhrase);
}

int ControlRecombinationState::Compare(const FFState& other) const
{
	const ControlRecombinationState &otherFF = static_cast<const ControlRecombinationState&>(other);
	bool ret = 	m_outputPhrase.Compare(otherFF.m_outputPhrase);
	return ret;
}


void ControlRecombination::Load()
{
}

std::vector<float> ControlRecombination::DefaultWeights() const
{
	CHECK(m_numScoreComponents == 1);
	vector<float> ret(1, 1);
	return ret;
}

FFState* ControlRecombination::Evaluate(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
}

FFState* ControlRecombination::EvaluateChart(
  const ChartHypothesis &hypo,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
}

void ControlRecombination::SetParameter(const std::string& key, const std::string& value)
{
}

}


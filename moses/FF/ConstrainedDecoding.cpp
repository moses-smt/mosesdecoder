#include "ConstrainedDecoding.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "util/exception.hh"

namespace Moses
{
ConstrainedDecodingState::ConstrainedDecodingState(const ChartHypothesis &hypo)
{
	hypo.CreateOutputPhrase(m_outputPhrase);
}

int ConstrainedDecodingState::Compare(const FFState& other) const
{
	const ConstrainedDecodingState &otherFF = static_cast<const ConstrainedDecodingState&>(other);
	bool ret = 	m_outputPhrase.Compare(otherFF.m_outputPhrase);
	return ret;
}

FFState* ConstrainedDecoding::Evaluate(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
	UTIL_THROW(util::Exception, "Not implemented");
}

FFState* ConstrainedDecoding::EvaluateChart(
  const ChartHypothesis &hypo,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
	const Phrase *ref = hypo.GetManager().GetConstraint();
	CHECK(ref);

	ConstrainedDecodingState *ret = new ConstrainedDecodingState(hypo);

	float score = ref->Contains(ret->GetPhrase()) ? 1 : - std::numeric_limits<float>::infinity();
	accumulator->PlusEquals(this, score);

	return ret;
}


}


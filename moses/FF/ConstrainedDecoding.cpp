#include "ConstrainedDecoding.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "util/exception.hh"

using namespace std;

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
	const ChartManager &mgr = hypo.GetManager();

	const Phrase *ref = mgr.GetConstraint();
	CHECK(ref);

	const Sentence &source = static_cast<const Sentence&>(mgr.GetSource());

	ConstrainedDecodingState *ret = new ConstrainedDecodingState(hypo);
	const Phrase &outputPhrase = ret->GetPhrase();

	size_t searchPos = ref->Find(outputPhrase);

	float score;
	if (hypo.GetCurrSourceRange().GetStartPos() == 0 &&
		hypo.GetCurrSourceRange().GetEndPos() == source.GetSize() - 1) {
		// translated entire sentence.
		score = (searchPos == 0) && (ref->GetSize() == outputPhrase.GetSize())
						? 0 : - std::numeric_limits<float>::infinity();
	}
	else {
		score = (searchPos != NOT_FOUND) ? 0 : - std::numeric_limits<float>::infinity();
	}

	accumulator->PlusEquals(this, score);

	return ret;
}


}


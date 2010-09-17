#include "TargetBigramFeature.h"

namespace Moses {

using namespace std;

TargetBigramFeature::TargetBigramFeature(ScoreIndexManager &scoreIndexManager);

size_t TargetBigramFeature::GetNumScoreComponents() const
{
	// TODO
	return 0;
}

string TargetBigramFeature::GetScoreProducerDescription() const
{
	// TODO
	return "";
}

string TargetBigramFeature::GetScoreProducerWeightShortName() const
{
	// TODO
	return "";
}

size_t TargetBigramFeature::GetNumInputScores() const
{
	// TODO
	return 0;
}


virtual const FFState* TargetBigramFeature::EmptyHypothesisState(const InputType &input) const
{
	// TODO
	return 0;
}

virtual FFState* TargetBigramFeature::Evaluate(const Hypothesis& cur_hypo,
                                               const FFState* prev_state,
                                               ScoreComponentCollection* accumulator) const
{
	// TODO
	return 0;
}

}

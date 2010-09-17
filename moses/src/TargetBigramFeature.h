#ifndef moses_TargetBigramFeature_h
#define moses_TargetBigramFeature_h
namespace Moses
{


/** Sets the features of observed bigrams.
 */
class TargetBigramFeature : public StatefulFeatureFunction {
public:
	TargetBigramFeature(ScoreIndexManager &scoreIndexManager);

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
	std::string GetScoreProducerWeightShortName() const;
	size_t GetNumInputScores() const;

	virtual const FFState* EmptyHypothesisState(const InputType &input) const;

	virtual FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state,
	                          ScoreComponentCollection* accumulator) const;
};

}

#endif // moses_TargetBigramFeature_h

// $Id$

#ifndef _DUMMY_SCORE_PRODUCERS_H_
#define _DUMMY_SCORE_PRODUCERS_H_

#include "FeatureFunction.h"

namespace Moses
{

class WordsRange;

/** Calculates Distortion scores
 */
class DistortionScoreProducer : public StatefulFeatureFunction {
public:
	DistortionScoreProducer(ScoreIndexManager &scoreIndexManager);

	float CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr, const int FirstGapPosition) const;

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
	std::string GetScoreProducerWeightShortName() const;
	size_t GetNumInputScores() const;

	virtual const FFState* EmptyHypothesisState() const;

  virtual FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

};

/** Doesn't do anything but provide a key into the global
 * score array to store the word penalty in.
 */
class WordPenaltyProducer : public StatelessFeatureFunction {
public:
	WordPenaltyProducer(ScoreIndexManager &scoreIndexManager);

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
	std::string GetScoreProducerWeightShortName() const;
	size_t GetNumInputScores() const;

	virtual void Evaluate(
		const TargetPhrase& phrase,
		ScoreComponentCollection* out) const;

};

/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction {
public:
	UnknownWordPenaltyProducer(ScoreIndexManager &scoreIndexManager);

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
	std::string GetScoreProducerWeightShortName() const;
	size_t GetNumInputScores() const;

	virtual bool ComputeValueInTranslationOption() const;

};

}

#endif

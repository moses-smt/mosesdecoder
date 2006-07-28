#ifndef _DUMMY_SCORE_PRODUCERS_H_
#define _DUMMY_SCORE_PRODUCERS_H_

#include "ScoreProducer.h"

class WordsRange;

/** Calculates Distortion scores
 */
struct DistortionScoreProducer : public ScoreProducer {
	DistortionScoreProducer();

	float CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr) const;

	unsigned int GetNumScoreComponents() const;
	const std::string GetScoreProducerDescription() const;
};

/** Doesn't do anything but provide a key into the global
 * score array to store the word penalty in.
 */
struct WordPenaltyProducer : public ScoreProducer {
	WordPenaltyProducer();

	unsigned int GetNumScoreComponents() const;
	const std::string GetScoreProducerDescription() const;
};

#endif

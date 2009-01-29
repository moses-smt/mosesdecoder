// $Id$

#ifndef _DUMMY_SCORE_PRODUCERS_H_
#define _DUMMY_SCORE_PRODUCERS_H_

#include "ScoreProducer.h"

namespace Moses
{

class WordsRange;
class Phrase;

/** Calculates WER scores
 */
class WERScoreProducer : public ScoreProducer {
public:
	WERScoreProducer(ScoreIndexManager &scoreIndexManager);

	float CalculateScore(const Phrase &hypPhrase, const Phrase &constraintPhrase) const;

	size_t GetNumScoreComponents() const
	{ return 1; }

	std::string GetScoreProducerDescription() const;
};

/** Calculates Distortion scores
 */
class DistortionScoreProducer : public ScoreProducer {
public:
	DistortionScoreProducer(ScoreIndexManager &scoreIndexManager);

	float CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr, const int FirstGapPosition) const;

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
};

/** Doesn't do anything but provide a key into the global
 * score array to store the word penalty in.
 */
class WordPenaltyProducer : public ScoreProducer {
public:
	WordPenaltyProducer(ScoreIndexManager &scoreIndexManager);

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
};

/** unknown word penalty */
class UnknownWordPenaltyProducer : public ScoreProducer {
public:
	UnknownWordPenaltyProducer(ScoreIndexManager &scoreIndexManager);

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
};

}

#endif

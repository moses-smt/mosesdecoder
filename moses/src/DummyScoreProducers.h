// $Id$

#ifndef moses_DummyScoreProducers_h
#define moses_DummyScoreProducers_h

#include "FeatureFunction.h"

namespace Moses
{

class WordsRange;

/** Calculates Distortion scores
 */
class DistortionScoreProducer : public StatefulFeatureFunction
{
public:
	DistortionScoreProducer() : StatefulFeatureFunction("Distortion", 1) {}

  float CalculateDistortionScore(const Hypothesis& hypo,
                                 const WordsRange &prev, const WordsRange &curr, const int FirstGapPosition) const;

	std::string GetScoreProducerWeightShortName(unsigned) const;
  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart(
    const ChartHypothesis&,
    int /* featureID */,
    ScoreComponentCollection*) const {
		CHECK(0); // feature function not valid in chart decoder
		return NULL;
	}
};

/** Doesn't do anything but provide a key into the global
 * score array to store the word penalty in.
 */
class WordPenaltyProducer : public StatelessFeatureFunction
{
public:
	WordPenaltyProducer() : StatelessFeatureFunction("WordPenalty",1) {}

	std::string GetScoreProducerWeightShortName(unsigned) const;

  virtual void Evaluate(
  	const Hypothesis& cur_hypo,
  	ScoreComponentCollection* accumulator) const;

  virtual void EvaluateChart(
    const ChartHypothesis&,
    int /* featureID */,
    ScoreComponentCollection*) const {
		// needs to be implemented but does nothing
	}
};

/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
public:
	UnknownWordPenaltyProducer() : StatelessFeatureFunction("!UnknownWordPenalty",1) {}

	std::string GetScoreProducerWeightShortName(unsigned) const;

  virtual bool ComputeValueInTranslationOption() const;

};

}

#endif

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
  DistortionScoreProducer(ScoreIndexManager &scoreIndexManager);

  float CalculateDistortionScore(const Hypothesis& hypo,
                                 const WordsRange &prev, const WordsRange &curr, const int FirstGapPosition) const;

  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

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
 *  score array to store the word penalty in.
 */
class WordPenaltyProducer : public StatelessFeatureFunction
{
public:
  WordPenaltyProducer(ScoreIndexManager &scoreIndexManager);

  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  virtual void Evaluate(
    const TargetPhrase& phrase,
    ScoreComponentCollection* out) const;
};

/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
public:
  UnknownWordPenaltyProducer(ScoreIndexManager &scoreIndexManager);

  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  virtual bool ComputeValueInTranslationOption() const;

};

}

#endif

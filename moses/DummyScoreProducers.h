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
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
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
	WordPenaltyProducer() : StatelessFeatureFunction("WordPenalty",1) {}

	std::string GetScoreProducerWeightShortName(unsigned) const;

  virtual void Evaluate(
    const PhraseBasedFeatureContext& context,
  	ScoreComponentCollection* accumulator) const;

  virtual void EvaluateChart(
    const ChartBasedFeatureContext& context,
    ScoreComponentCollection* accumulator) const 
    {
      //required but does nothing.
    }



};

/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
public:
	UnknownWordPenaltyProducer() : StatelessFeatureFunction("!UnknownWordPenalty",1) {}

	std::string GetScoreProducerWeightShortName(unsigned) const;

  virtual bool ComputeValueInTranslationOption() const;
  void Evaluate(  const PhraseBasedFeatureContext& context,
  								ScoreComponentCollection* accumulator) const 
  {
    //do nothing - not a real feature
  }

  void EvaluateChart(
    const ChartBasedFeatureContext& context,
    ScoreComponentCollection* accumulator) const
  {
    //do nothing - not a real feature
  }

  bool ComputeValueInTranslationTable() const {return true;}

};

class MetaFeatureProducer : public StatelessFeatureFunction
{
 public:
 MetaFeatureProducer(std::string shortName) : StatelessFeatureFunction("MetaFeature_"+shortName,1), m_shortName(shortName) {}

  std::string m_shortName;
  
  std::string GetScoreProducerWeightShortName(unsigned) const;

  void Evaluate(const PhraseBasedFeatureContext& context,
		ScoreComponentCollection* accumulator) const {
    //do nothing - not a real feature
  }

  void EvaluateChart(const ChartBasedFeatureContext& context,
		     ScoreComponentCollection*) const {
    //do nothing - not a real feature
  }
};

}

#endif

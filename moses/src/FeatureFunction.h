#ifndef moses_FeatureFunction_h
#define moses_FeatureFunction_h

#include <vector>

#include "ScoreProducer.h"

namespace Moses
{

class TargetPhrase;
class TranslationOption;
class Hypothesis;
class ChartHypothesis;
class FFState;
class InputType;
class ScoreComponentCollection;
class WordsBitmap;
class WordsRange;


/**
  * Contains all that a feature function can access without affecting recombination.
  * For stateless features, this is all that it can access. Currently this is not
  * used for stateful features, as it would need to be retro-fitted to the LM feature.
  * TODO: Expose source segmentation,lattice path.
  * XXX Don't add anything to the context that would break recombination XXX
 **/
class PhraseBasedFeatureContext
{
  // The context either has a hypothesis (during search), or a TranslationOption and 
  // source sentence (during pre-calculation).
  const Hypothesis* m_hypothesis;
  const TranslationOption& m_translationOption;
  const InputType& m_source;

public:
  PhraseBasedFeatureContext(const Hypothesis* hypothesis);
  PhraseBasedFeatureContext(const TranslationOption& translationOption,
                            const InputType& source);

  const TranslationOption& GetTranslationOption() const;
  const InputType& GetSource() const;
  const TargetPhrase& GetTargetPhrase() const; //convenience method
  const WordsBitmap& GetWordsBitmap() const;

};

/**
 * Same as PhraseBasedFeatureContext, but for chart-based Moses.
 **/
class ChartBasedFeatureContext
{
  //The context either has a hypothesis (during search) or a 
  //TargetPhrase and source sentence (during pre-calculation)
  //TODO: should the context also include some info on where the TargetPhrase
  //is anchored (assuming it's lexicalised), which is available at pre-calc?
  const ChartHypothesis* m_hypothesis;
  const TargetPhrase& m_targetPhrase;
  const InputType& m_source;

public:
  ChartBasedFeatureContext(const ChartHypothesis* hypothesis);
  ChartBasedFeatureContext(const TargetPhrase& targetPhrase,
                           const InputType& source);

  const InputType& GetSource() const;
  const TargetPhrase& GetTargetPhrase() const;

};


/** base class for all feature functions.
 * @todo is this for pb & hiero too?
 * @todo what's the diff between FeatureFunction and ScoreProducer?
 */
class FeatureFunction: public ScoreProducer
{

public:
  FeatureFunction(const std::string& description, size_t numScoreComponents) :
    ScoreProducer(description, numScoreComponents) {}
  virtual bool IsStateless() const = 0;	
  virtual ~FeatureFunction();
  
  float GetSparseProducerWeight() const { return 1; }	
};

/** base class for all stateless feature functions.
 * eg. phrase table, word penalty, phrase penalty
 */
class StatelessFeatureFunction: public FeatureFunction
{

public:
  StatelessFeatureFunction(const std::string& description, size_t numScoreComponents) :
    FeatureFunction(description, numScoreComponents) {}
  /**
    * This should be implemented for features that apply to phrase-based models.
    **/
  virtual void Evaluate(const PhraseBasedFeatureContext& context,
  											ScoreComponentCollection* accumulator) const = 0;

  /**
    * Same for chart-based features.
    **/
  virtual void EvaluateChart(const ChartBasedFeatureContext& context,
                             ScoreComponentCollection* accumulator) const  = 0;

  //If true, then the feature is evaluated before search begins, and stored in
  //the TranslationOptionCollection.
  virtual bool ComputeValueInTranslationOption() const;

  //!If true, the feature is stored in the ttable, so gets copied into the 
  //TargetPhrase and does not need cached in the TranslationOption
  virtual bool ComputeValueInTranslationTable() const {return false;}

  bool IsStateless() const;
};

/** base class for all stateful feature functions.
 * eg. LM, distortion penalty 
 */
class StatefulFeatureFunction: public FeatureFunction
{

public:
  StatefulFeatureFunction(const std::string& description, size_t numScoreComponents) :
    FeatureFunction(description,numScoreComponents) {}

  /**
   * \brief This interface should be implemented.
   * Notes: When evaluating the value of this feature function, you should avoid
   * calling hypo.GetPrevHypo().  If you need something from the "previous"
   * hypothesis, you should store it in an FFState object which will be passed
   * in as prev_state.  If you don't do this, you will get in trouble.
   */
  virtual FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const = 0;

  virtual FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const = 0;

  //! return the state associated with the empty hypothesis for a given sentence
  virtual const FFState* EmptyHypothesisState(const InputType &input) const = 0;

  bool IsStateless() const;
};

}

#endif

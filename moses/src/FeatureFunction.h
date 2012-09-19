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
    * The feature is allowed to access the translation option, source and 
    * coverage vector, but the last can only be accessed during search.
    **/
  virtual void Evaluate(const PhraseBasedFeatureContext& context,
  											ScoreComponentCollection* accumulator) const = 0;

  virtual void EvaluateChart(const TargetPhrase& targetPhrase,
                             const InputType& inputType,
                             const WordsRange& sourceSpan,
                             ScoreComponentCollection* accumulator) const  = 0;

  //If true, then the feature is evaluated before search begins, and stored in
  //the TranslationOptionCollection. Note that for PhraseDictionary and 
  //GenerationDictionary the scores are actually read from the TargetPhrase
  virtual bool ComputeValueInTranslationOption() const;

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

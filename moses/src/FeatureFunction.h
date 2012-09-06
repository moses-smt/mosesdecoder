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
  virtual void Evaluate(const TranslationOption& translationOption,
                        const InputType& inputType,
                        const WordsBitmap& coverageVector,
  											ScoreComponentCollection* accumulator) const {}
                        //TODO: Warn if unimplemented

  virtual void EvaluateChart(const ChartHypothesis& cur_hypo,
  													 int featureID,
                             ScoreComponentCollection* accumulator) const {}
                        //TODO: Warn if unimplemented

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
    int /* featureID */,
    ScoreComponentCollection* accumulator) const = 0;

  //! return the state associated with the empty hypothesis for a given sentence
  virtual const FFState* EmptyHypothesisState(const InputType &input) const = 0;

  bool IsStateless() const;
};

}

#endif

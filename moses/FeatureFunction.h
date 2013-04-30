#ifndef moses_FeatureFunction_h
#define moses_FeatureFunction_h

#include <vector>
#include <set>
#include <string>
#include "TypeDef.h"

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
 */
class FeatureFunction
{
protected:
  /**< all the score producers in this run */
  static std::vector<FeatureFunction*> m_producers;

  std::string m_description, m_argLine;
  std::vector<std::vector<std::string> > m_args;
  bool m_reportSparseFeatures;
  size_t m_numScoreComponents;
  //In case there's multiple producers with the same description
  static std::multiset<std::string> description_counts;

  void ParseLine(const std::string& description, const std::string &line);
  size_t FindNumFeatures();
  bool FindName();

public:
  static const std::vector<FeatureFunction*>& GetFeatureFunctions() { return m_producers; }

  FeatureFunction(const std::string& description, const std::string &line);
  FeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line);
  virtual bool IsStateless() const = 0;	
  virtual ~FeatureFunction();
  
  static const size_t unlimited;

  static void ResetDescriptionCounts() {
    description_counts.clear();
  }

  //! returns the number of scores that a subclass produces.
  //! For example, a language model conventionally produces 1, a translation table some arbitrary number, etc
  //! sparse features returned unlimited
  size_t GetNumScoreComponents() const {return m_numScoreComponents;}

  //! returns a string description of this producer
  const std::string& GetScoreProducerDescription() const
  { return m_description; }

  void SetSparseFeatureReporting() { m_reportSparseFeatures = true; }
  bool GetSparseFeatureReporting() const { return m_reportSparseFeatures; }

  virtual float GetSparseProducerWeight() const { return 1; }

  virtual bool IsTuneable() const { return true; }

  //!
  virtual void InitializeForInput(InputType const& source)
  {}

  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing(const InputType& source)
  {}

  const std::string &GetArgLine() const
  { return m_argLine; }

};

/** base class for all stateless feature functions.
 * eg. phrase table, word penalty, phrase penalty
 */
class StatelessFeatureFunction: public FeatureFunction
{
  //All stateless FFs, except those that cache scores in T-Option
  static std::vector<const StatelessFeatureFunction*> m_statelessFFs;

public:
  static const std::vector<const StatelessFeatureFunction*>& GetStatelessFeatureFunctions() {return m_statelessFFs;}

  StatelessFeatureFunction(const std::string& description, const std::string &line);
  StatelessFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line);
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

  virtual StatelessFeatureType GetStatelessFeatureType() const
  { return CacheableInPhraseTable; }

  bool IsStateless() const
  { return true; }

};

/** base class for all stateful feature functions.
 * eg. LM, distortion penalty 
 */
class StatefulFeatureFunction: public FeatureFunction
{
  //All statefull FFs
  static std::vector<const StatefulFeatureFunction*> m_statefulFFs;

public:
  static const std::vector<const StatefulFeatureFunction*>& GetStatefulFeatureFunctions() {return m_statefulFFs;}

  StatefulFeatureFunction(const std::string& description, const std::string &line);
  StatefulFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line);

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

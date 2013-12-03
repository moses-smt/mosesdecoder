#ifndef moses_FeatureFunction_h
#define moses_FeatureFunction_h

#include <vector>
#include <set>
#include <string>
#include "moses/TypeDef.h"

namespace Moses
{

class Phrase;
class TargetPhrase;
class TranslationOption;
class Hypothesis;
class ChartHypothesis;
class InputType;
class ScoreComponentCollection;
class WordsBitmap;
class WordsRange;
class FactorMask;
class InputPath;

/** base class for all feature functions.
 */
class FeatureFunction
{
protected:
  /**< all the score producers in this run */
  static std::vector<FeatureFunction*> s_staticColl;

  std::string m_description, m_argLine;
  std::vector<std::vector<std::string> > m_args;
  bool m_tuneable;
  size_t m_numScoreComponents;
  //In case there's multiple producers with the same description
  static std::multiset<std::string> description_counts;

  void Initialize(const std::string &line);
  void ParseLine(const std::string &line);

public:
  static const std::vector<FeatureFunction*>& GetFeatureFunctions() {
    return s_staticColl;
  }
  static FeatureFunction &FindFeatureFunction(const std::string& name);

  FeatureFunction(const std::string &line);
  FeatureFunction(size_t numScoreComponents, const std::string &line);
  virtual bool IsStateless() const = 0;
  virtual ~FeatureFunction();

  //! override to load model files
  virtual void Load() {
  }

  static void ResetDescriptionCounts() {
    description_counts.clear();
  }

  //! returns the number of scores that a subclass produces.
  //! For example, a language model conventionally produces 1, a translation table some arbitrary number, etc
  size_t GetNumScoreComponents() const {
    return m_numScoreComponents;
  }

  //! returns a string description of this producer
  const std::string& GetScoreProducerDescription() const {
    return m_description;
  }

  //! if false, then this feature is not displayed in the n-best list.
  // use with care
  virtual bool IsTuneable() const {
    return m_tuneable;
  }
  virtual std::vector<float> DefaultWeights() const;

  //! Called before search and collecting of translation options
  virtual void InitializeForInput(InputType const& source) {
  }

  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing(const InputType& source) {
  }

  const std::string &GetArgLine() const {
    return m_argLine;
  }

  // given a target phrase containing only factors specified in mask
  // return true if the feature function can be evaluated
  virtual bool IsUseable(const FactorMask &mask) const = 0;

  // used by stateless ff and stateful ff. Calculate initial score estimate during loading of phrase table
  // source phrase is the substring that the phrase table uses to look up the target phrase,
  // may have more factors than actually need, but not guaranteed.
  // For SCFG decoding, the source contains non-terminals, NOT the raw source from the input sentence
  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const = 0;

  // This method is called once all the translation options are retrieved from the phrase table, and
  // just before search.
  // 'inputPath' is guaranteed to be the raw substring from the input. No factors were added or taken away
  // Currently not used by any FF. Not called by moses_chart
  // No FF should set estimatedFutureScore in both overloads!
  virtual void Evaluate(const InputType &input
                        , const InputPath &inputPath
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection *estimatedFutureScore = NULL) const = 0;

  virtual void SetParameter(const std::string& key, const std::string& value);
  virtual void ReadParameters();
};

}

#endif

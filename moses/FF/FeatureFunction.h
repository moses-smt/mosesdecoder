#ifndef moses_FeatureFunction_h
#define moses_FeatureFunction_h

#include <vector>
#include <set>
#include <string>
#include "PhraseBasedFeatureContext.h"
#include "ChartBasedFeatureContext.h"
#include "moses/TypeDef.h"

namespace Moses
{

class Phrase;
class TargetPhrase;
class TranslationOption;
class Hypothesis;
class ChartHypothesis;
class FFState;
class InputType;
class ScoreComponentCollection;
class WordsBitmap;
class WordsRange;
class FactorMask;



/** base class for all feature functions.
 */
class FeatureFunction
{
protected:
  /**< all the score producers in this run */
  static std::vector<FeatureFunction*> m_producers;

  std::string m_description, m_argLine;
  std::vector<std::vector<std::string> > m_args;
  bool m_tuneable;
  size_t m_numScoreComponents;
  //In case there's multiple producers with the same description
  static std::multiset<std::string> description_counts;

  void ParseLine(const std::string& description, const std::string &line);

public:
  static const std::vector<FeatureFunction*>& GetFeatureFunctions() {
    return m_producers;
  }
  static FeatureFunction &FindFeatureFunction(const std::string& name);

  FeatureFunction(const std::string& description, const std::string &line);
  FeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line);
  virtual bool IsStateless() const = 0;
  virtual ~FeatureFunction();

  virtual void Load()
  {}

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

  virtual bool IsTuneable() const {
    return m_tuneable;
  }

  //!
  virtual void InitializeForInput(InputType const& source)
  {}

  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing(const InputType& source)
  {}

  const std::string &GetArgLine() const {
    return m_argLine;
  }

  virtual bool IsUseable(const FactorMask &mask) const = 0;

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const
  {}

  virtual void Evaluate(const InputType &source
                        , ScoreComponentCollection &scoreBreakdown) const
  {}

  virtual void OverrideParameter(const std::string& key, const std::string& value);
};

}

#endif

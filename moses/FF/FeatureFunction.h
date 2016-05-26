// -*- c++ -*-
#ifndef moses_FeatureFunction_h
#define moses_FeatureFunction_h

#include <vector>
#include <set>
#include <string>
#include "moses/FeatureVector.h"
#include "moses/TypeDef.h"
#include "moses/parameters/AllOptions.h"
#include <boost/shared_ptr.hpp>

namespace Moses
{

class AllOptions;
class Phrase;
class TargetPhrase;
class TranslationOptionList;
class TranslationOption;
class Hypothesis;
class ChartHypothesis;
class InputType;
class ScoreComponentCollection;
class Bitmap;
class Range;
class FactorMask;
class InputPath;
class StackVec;
class DistortionScoreProducer;
class TranslationTask;

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
  bool m_requireSortingAfterSourceContext;
  size_t m_verbosity;
  size_t m_numScoreComponents;
  size_t m_index; // index into vector covering ALL feature function values
  std::vector<bool> m_tuneableComponents;
  size_t m_numTuneableComponents;
  AllOptions::ptr m_options;
  //In case there's multiple producers with the same description
  static std::multiset<std::string> description_counts;

public:
  static void Register(FeatureFunction* ff);
private:
  // void Initialize(const std::string &line);
  void ParseLine(const std::string &line);

public:
  static const std::vector<FeatureFunction*>& GetFeatureFunctions() {
    return s_staticColl;
  }

  static FeatureFunction &FindFeatureFunction(const std::string& name);
  static void Destroy();

  FeatureFunction(const std::string &line, bool registerNow);
  FeatureFunction(size_t numScoreComponents, const std::string &line, bool registerNow = true);
  virtual bool IsStateless() const = 0;
  virtual ~FeatureFunction();

  //! override to load model files
  virtual void Load(AllOptions::ptr const& opts) {
    m_options = opts;
  }

  AllOptions::ptr const&
  options() const {
    return m_options;
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

  FName GetFeatureName(const std::string& name) const {
    return FName(GetScoreProducerDescription(), name);
  }


  //! if false, then this feature is not displayed in the n-best list.
  // use with care
  virtual bool IsTuneable() const {
    return m_tuneable;
  }

  virtual bool HasTuneableComponents() const {
    return m_numTuneableComponents;
  }

  virtual bool IsTuneableComponent(size_t i) const {
    if (m_numTuneableComponents == m_numScoreComponents) {
      return true;
    }
    return m_tuneableComponents[i];
  }

  virtual bool RequireSortingAfterSourceContext() const {
    return m_requireSortingAfterSourceContext;
  }

  virtual std::vector<float> DefaultWeights() const;

  size_t GetIndex() const;
  size_t SetIndex(size_t const idx);

protected:
  virtual void
  CleanUpAfterSentenceProcessing(InputType const& source) { }

public:
  //! Called before search and collecting of translation options
  virtual void
  InitializeForInput(ttasksptr const& ttask) { };

  // clean up temporary memory, called after processing each sentence
  virtual void
  CleanUpAfterSentenceProcessing(ttasksptr const& ttask);

  const std::string &
  GetArgLine() const {
    return m_argLine;
  }

  // given a target phrase containing only factors specified in mask
  // return true if the feature function can be evaluated
  virtual bool IsUseable(const FactorMask &mask) const = 0;

  // used by stateless ff and stateful ff. Calculate initial score
  // estimate during loading of phrase table
  //
  // source phrase is the substring that the phrase table uses to look
  // up the target phrase,
  //
  // may have more factors than actually need, but not guaranteed.
  // For SCFG decoding, the source contains non-terminals, NOT the raw
  // source from the input sentence
  virtual void
  EvaluateInIsolation(const Phrase &source, const TargetPhrase &targetPhrase,
                      ScoreComponentCollection& scoreBreakdown,
                      ScoreComponentCollection& estimatedScores) const = 0;

  // for context-dependent processing
  static void SetupAll(TranslationTask const& task);
  virtual void Setup(TranslationTask const& task) const { };

  // This method is called once all the translation options are retrieved from the phrase table, and
  // just before search.
  // 'inputPath' is guaranteed to be the raw substring from the input. No factors were added or taken away
  // 'stackVec' is a vector of chart cells that the RHS non-terms cover.
  // It is guaranteed to be in the same order as the non-terms in the source phrase.
  // For pb models, stackvec is NULL.
  // No FF should set estimatedScores in both overloads!
  virtual void EvaluateWithSourceContext(const InputType &input
                                         , const InputPath &inputPath
                                         , const TargetPhrase &targetPhrase
                                         , const StackVec *stackVec
                                         , ScoreComponentCollection &scoreBreakdown
                                         , ScoreComponentCollection *estimatedScores = NULL) const = 0;

  // This method is called once all the translation options are retrieved from the phrase table, and
  // just before search.
  // 'inputPath' is guaranteed to be the raw substring from the input. No factors were added or taken away
  // 'stackVec' is a vector of chart cells that the RHS non-terms cover.
  // It is guaranteed to be in the same order as the non-terms in the source phrase.
  // For pb models, stackvec is NULL.
  // No FF should set estimatedScores in both overloads!
  virtual void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const = 0;

  virtual void SetParameter(const std::string& key, const std::string& value);
  virtual void ReadParameters();
  virtual void SetTuneableComponents(const std::string& value);
};

}

#endif

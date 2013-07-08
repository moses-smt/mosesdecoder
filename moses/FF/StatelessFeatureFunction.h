#pragma once

#include "FeatureFunction.h"

namespace Moses
{

/** base class for all stateless feature functions.
 * eg. phrase table, word penalty, phrase penalty
 */
class StatelessFeatureFunction: public FeatureFunction
{
  //All stateless FFs, except those that cache scores in T-Option
  static std::vector<const StatelessFeatureFunction*> m_statelessFFs;

public:
  static const std::vector<const StatelessFeatureFunction*>& GetStatelessFeatureFunctions() {
    return m_statelessFFs;
  }

  StatelessFeatureFunction(const std::string& description, const std::string &line);
  StatelessFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line);
  /**
    * This should be implemented for features that apply to phrase-based models.
    **/
  virtual void Evaluate(const PhraseBasedFeatureContext& context,
                        ScoreComponentCollection* accumulator) const {
  }

  /**
    * Same for chart-based features.
    **/
  virtual void EvaluateChart(const ChartBasedFeatureContext& context,
                             ScoreComponentCollection* accumulator) const {
  }

  /**
   * Implement this if your feature can be computed before decoding
   * (when translation options are collected). You will get access to all phrase
   * translations at once as a reward. Override ComputedInTranslationOption()
   * and return true.
   */
  virtual std::vector<ScoreComponentCollection> EvaluateOptions(
      const std::vector<TranslationOption *> &options, const InputType &src) const {
    return std::vector<ScoreComponentCollection>(); // empty 
  }

  virtual bool IsStateless() const {
    return true;
  }

  virtual bool ComputedInTranslationOption() const {
    return false;
  }

};


} // namespace


#pragma once

#include "FeatureFunction.h"


namespace Moses
{

namespace Syntax
{
struct SHyperedge;
}

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

  StatelessFeatureFunction(const std::string &line, bool registerNow);
  StatelessFeatureFunction(size_t numScoreComponents, const std::string &line);

  /**
    * This should be implemented for features that apply to phrase-based models.
    **/
  virtual void EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const = 0;

  /**
    * Same for chart-based features.
    **/
  virtual void EvaluateWhenApplied(const ChartHypothesis &hypo,
                                   ScoreComponentCollection* accumulator) const = 0;

  virtual void EvaluateWhenApplied(const Syntax::SHyperedge &,
                                   ScoreComponentCollection*) const {
    assert(false);
  }

  virtual bool IsStateless() const {
    return true;
  }

};


} // namespace


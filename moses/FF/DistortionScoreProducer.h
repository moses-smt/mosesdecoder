#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "moses/Range.h"

namespace Moses
{

/** Calculates Distortion scores
 */
class DistortionScoreProducer : public StatefulFeatureFunction
{
protected:
  static std::vector<const DistortionScoreProducer*> s_staticColl;

  FactorType m_sparseFactorTypeSource;
  FactorType m_sparseFactorTypeTarget;
  bool m_useSparse;
  bool m_sparseDistance;
  bool m_sparseSubordinate;
  FactorType m_sparseFactorTypeTargetSubordinate;
  const Factor* m_subordinateConjunctionTagFactor;

public:
  static const std::vector<const DistortionScoreProducer*>& GetDistortionFeatureFunctions() {
    return s_staticColl;
  }

  DistortionScoreProducer(const std::string &line);

  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  static float CalculateDistortionScore(const Hypothesis& hypo,
                                        const Range &prev, const Range &curr, const int FirstGapPosition);

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection*) const {
    UTIL_THROW(util::Exception, "DIstortion not implemented in chart decoder");
  }

};
}


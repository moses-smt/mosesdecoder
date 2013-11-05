
#pragma once

#include "FeatureFunction.h"

class FFState;
class Hypothesis;
class Scores;

class StatefulFeatureFunction : public FeatureFunction
{
public:
  static const std::vector<StatefulFeatureFunction*>& GetColl() {
    return s_staticColl;
  }
  static void Evaluate(Hypothesis& hypo);
  static void EvaluateEmptyHypo(const Sentence &input, Hypothesis& hypo);

  StatefulFeatureFunction(const std::string line);
  virtual ~StatefulFeatureFunction();

  virtual size_t Evaluate(
    const Hypothesis& hypo,
    size_t prevState,
    Scores &scores) const = 0;
  virtual size_t EmptyHypo(
    const Sentence &input,
    Hypothesis& hypo) const {
    return 0;
  }

protected:
  static std::vector<StatefulFeatureFunction*> s_staticColl;

};


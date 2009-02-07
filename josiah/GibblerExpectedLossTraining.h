#pragma once

#include <utility>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

using namespace Moses;

namespace Josiah {

class GainFunction;

class GibblerExpectedLossCollector : public SampleCollector {
 public:
  GibblerExpectedLossCollector(const GainFunction& f) : g(f), n(0) {}
  virtual void collect(Sample& sample);

  void UpdateGradient(ScoreComponentCollection* gradient);

 private:
  const GainFunction& g;
  int n;
  std::list<std::pair<ScoreComponentCollection, float> > samples;
  ScoreComponentCollection feature_expectations;
};

}

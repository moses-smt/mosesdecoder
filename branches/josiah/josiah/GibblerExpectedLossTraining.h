#pragma once

#include <map>
#include <utility>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

using namespace Moses;

namespace Josiah {

class GainFunction;

class GibblerExpectedLossCollector : public SampleCollector {
 public:
  GibblerExpectedLossCollector(const GainFunction& f) : g(f), n(), tot_len() {}
  virtual void collect(Sample& sample);

  // returns the expected gain and expected sentence length
  float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len);

 private:
  const GainFunction& g;
  int n;
  size_t tot_len;
  std::list<std::pair<ScoreComponentCollection, float> > samples;
  ScoreComponentCollection feature_expectations;
};

}

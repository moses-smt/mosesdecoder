#pragma once

#include <map>
#include <utility>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

using namespace Moses;

namespace Josiah {

class GainFunction;

class ExpectedLossCollector : public SampleCollector {
  public:
    ExpectedLossCollector(const GainFunction& f) : g(f), n(), tot_len() {}
    virtual ~ExpectedLossCollector() {}
    virtual void collect(Sample& sample) = 0;
    // returns the expected gain and expected sentence length
    virtual float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len) = 0;
    
    
  protected:
    const GainFunction& g;
    int n;
    size_t tot_len;
    ScoreComponentCollection feature_expectations;
};
  
  
  
class GibblerExpectedLossCollector : public ExpectedLossCollector {
 public:
  GibblerExpectedLossCollector(const GainFunction& f) : ExpectedLossCollector(f) {}
  virtual void collect(Sample& sample);
  // returns the expected gain and expected sentence length
  virtual float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len);
  
 private:
  std::list<std::pair<ScoreComponentCollection, float> > samples;
};

}

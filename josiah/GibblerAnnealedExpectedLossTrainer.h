#pragma once

#include <map>
#include <utility>
#include <ext/hash_map>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Derivation.h"
#include "GibblerExpectedLossTraining.h"
#include "Phrase.h"

using namespace Moses;

namespace Josiah {
  
  class GainFunction;
  
  class GibblerAnnealedExpectedLossCollector : public ExpectedLossCollector {
  public:
    GibblerAnnealedExpectedLossCollector(const GainFunction& f) :  ExpectedLossCollector(f) {}
    
    virtual void collect(Sample& sample);
    // returns the expected gain and expected sentence length
    virtual float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len);
    float ComputeEntropy();
    float GetTemperature() { return m_temp;}
    void SetTemperature(float temp) {m_temp = temp;} 
    
  private:
    float m_temp;
    std::map<Derivation,size_t> m_counts;
    std::map<Derivation,float> m_gain;
  };
  
}

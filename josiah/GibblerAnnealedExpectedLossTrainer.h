#pragma once

#include <map>
#include <utility>
#include <ext/hash_map>

#include "ScoreComponentCollection.h"
#include "Derivation.h"
#include "GibblerExpectedLossTraining.h"
#include "Phrase.h"
#include "Sampler.h"
#include "GibblerMaxDerivDecoder.h"

using namespace Moses;

namespace Josiah {
  
  class GainFunction;

  
  class GibblerAnnealedExpectedLossCollector : public ExpectedLossCollector {
  public:
    GibblerAnnealedExpectedLossCollector(const GainFunction* f, Sampler& sampler) 
      :  ExpectedLossCollector(f) {
        sampler.AddCollector(&m_derivationCollector);
      }
    
    float ComputeEntropy();
    float GetTemperature() { return m_temp;}
    void SetTemperature(float temp) {m_temp = temp;} 
    virtual float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len, float * unreg_exp_gain);
    virtual float getRegularisationGradientFactor(size_t i);
    virtual float getRegularisation();

    
  private:
    float m_temp;
    DerivationCollector m_derivationCollector;
    
    //cache the distribution during gradient calculation
    std::map<const Derivation*,double> m_p;
    
    
  };
  
}

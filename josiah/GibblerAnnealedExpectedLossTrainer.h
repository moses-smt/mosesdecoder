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
    GibblerAnnealedExpectedLossCollector(const GainFunctionHandle& gain, Sampler& sampler) 
      :  ExpectedLossCollector(gain) {
        sampler.AddCollector(&m_derivationCollector);
      }
    
    FValue ComputeEntropy();
    FValue GetTemperature() { return m_temp;}
    void SetTemperature(FValue temp) {m_temp = temp;} 
    virtual FValue UpdateGradient(FVector* gradient, FValue* exp_len, FValue* unreg_exp_gain);
    virtual FValue getRegularisationGradientFactor(size_t i);
    virtual FValue getRegularisation();

    
  private:
    float m_temp;
    DerivationCollector m_derivationCollector;
    
    //cache the distribution during gradient calculation
    std::map<const Derivation*,double> m_p;
    
    
  };
  
}

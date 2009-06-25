#pragma once

#include <map>
#include <utility>
#include <ext/hash_map>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Derivation.h"
#include "CorpusSampler.h"
#include "GibblerMaxDerivDecoder.h"
#include "Phrase.h"
#ifdef MPI_ENABLED
#include <mpi.h>
#endif


using namespace Moses;

namespace Josiah {
  
  class GainFunction;
  
  class CorpusSamplerAnnealedCollector : public CorpusSamplerCollector {
  public:
    CorpusSamplerAnnealedCollector(int samples, Sampler &sampler) 
    :  CorpusSamplerCollector(samples, sampler), m_regularisation(0.0) {
    }
    
    float GetTemperature() { return m_temp;}
    void SetTemperature(float temp) {m_temp = temp;} 
    virtual ScoreComponentCollection getRegularisationGradientFactor() {
      return m_gradient;
    }
    virtual float getRegularisation() {
      return m_regularisation;
    }
    virtual void reset() {
      CorpusSamplerCollector::reset();
      m_regularisation = 0.0;
      m_gradient.ZeroAll();
    }
    virtual void setRegularisationGradientFactor(std::map<const Derivation*,double>& m_p);
    virtual void setRegularisation(std::map<const Derivation*,double>& m_p);
#ifdef MPI_ENABLED
    virtual void AggregateSamples(int rank);
#endif
  private:
    float m_temp, m_regularisation;
    ScoreComponentCollection m_gradient;
    ScoreComponentCollection getExpectedFeatureValue(std::map<const Derivation*,double>& m_p);
#ifdef MPI_ENABLED
    void AggregateRegularisationStats(int rank);
#endif
    
  };
  
}

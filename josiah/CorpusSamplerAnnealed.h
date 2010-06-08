#pragma once

#include <map>
#include <utility>
#include <ext/hash_map>

#include "FeatureVector.h"
#include "CorpusSampler.h"
#include "Phrase.h"
#ifdef MPI_ENABLED
#include <mpi.h>
#endif


using namespace Moses;

namespace Josiah {
  class Derivation;
  class GainFunction;
  class CorpusSamplerCollector;
  
  class CorpusSamplerAnnealedCollector : public CorpusSamplerCollector {
  public:
    CorpusSamplerAnnealedCollector(int samples, Sampler &sampler) 
    :  CorpusSamplerCollector(samples, sampler), m_regularisation(0.0) {
    }
    
    float GetTemperature() { return m_temp;}
    void SetTemperature(float temp) {m_temp = temp;} 
    virtual FVector getRegularisationGradientFactor() {
      return m_gradient;
    }
    virtual float getRegularisation() {
      return m_regularisation;
    }
    virtual void reset() {
      CorpusSamplerCollector::reset();
      m_regularisation = 0.0;
      m_gradient.clear();
    }
    virtual void setRegularisationGradientFactor(std::map<const Derivation*,double>& m_p);
    virtual void setRegularisation(std::map<const Derivation*,double>& m_p);
#ifdef MPI_ENABLED
    virtual void AggregateSamples(int rank);
#endif
  private:
    FValue m_temp, m_regularisation;
    FVector m_gradient;
    FVector getExpectedFeatureValue(std::map<const Derivation*,double>& m_p);
#ifdef MPI_ENABLED
    void AggregateRegularisationStats(int rank);
#endif
    
  };
  
}

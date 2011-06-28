#pragma once

#include <map>
#include <utility>

#include "MpiDebug.h"
#include "FeatureVector.h"
#include "GibblerExpectedLossTraining.h"
#include "GibblerMaxDerivDecoder.h"
#include "Phrase.h"
#include "Sampler.h"


#ifdef MPI_ENABLED
#include <mpi.h>
#endif

using namespace Moses;

namespace Josiah {
  
  class Sampler;
  class Derivation;  
  class CorpusSamplerCollector : public ExpectedLossCollector {
  public:
      CorpusSamplerCollector(int samples, Sampler &sampler):  ExpectedLossCollector(),
          m_samples(samples), m_numSents(0)  {
      sampler.AddCollector(&m_derivationCollector);
      m_featureVectors.resize(m_samples);
      m_lengths.resize(m_samples);
      m_sufficientStats.resize(m_samples);
    }
    virtual ~CorpusSamplerCollector() {}
    virtual void collect(Sample& sample);
    virtual void resample(int);
    virtual FValue UpdateGradient(FVector* gradient, FValue* exp_len, FValue* unreg_exp_gain);
#ifdef MPI_ENABLED  
    virtual void AggregateSamples(int);
#endif
    virtual void reset();   
    float getReferenceLength();
    virtual void setRegularisationGradientFactor(std::map<const Derivation*,double>& m_p) {}
    virtual void setRegularisation(std::map<const Derivation*,double>& m_p) {}
    virtual FVector getRegularisationGradientFactor() {return FVector();}
    virtual FValue getRegularisation() {return 0.0;}
    
  private:
    std::vector<FVector> m_featureVectors;
    std::vector  <int>  m_lengths;
    std::vector <BleuSufficientStats> m_sufficientStats; 
    
    DerivationCollector m_derivationCollector;
    const int m_samples;
    FVector getFeatureExpectations() const;
    int m_numSents;
    int GetNumSents() { return m_numSents;}
  protected:   
    void AggregateSuffStats(int);    
    
  };
  
}

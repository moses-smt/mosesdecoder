#pragma once

#include <map>
#include <utility>

#include "Derivation.h"
#include "Gibbler.h"
#include "MpiDebug.h"
#include "ScoreComponentCollection.h"
#include "GibblerExpectedLossTraining.h"
#include "GibblerMaxDerivDecoder.h"
#include "Phrase.h"
#include "SentenceBleu.h"
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

using namespace Moses;

namespace Josiah {
  
  class GainFunction;
  
  class CorpusSamplerCollector : public ExpectedLossCollector {
  public:
    CorpusSamplerCollector(int samples, Sampler &sampler) 
    :  m_samples(samples), m_numSents(0), ExpectedLossCollector() {
      sampler.AddCollector(&m_derivationCollector);
      m_featureVectors.resize(m_samples);
      m_lengths.resize(m_samples);
      m_sufficientStats.resize(m_samples);
    }
    virtual ~CorpusSamplerCollector() {}
    virtual void collect(Sample& sample);
    virtual void resample(int);
    virtual float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len, float * unreg_exp_gain, float *scaling_gradient);
    virtual void UpdateHessianVProduct(ScoreComponentCollection* hessian, const ScoreComponentCollection& v) {}
#ifdef MPI_ENABLED  
    virtual void AggregateSamples(int);
#endif
    virtual void reset();   
    float getReferenceLength();
    virtual void setRegularisationGradientFactor(std::map<const Derivation*,double>& m_p) {}
    virtual void setRegularisation(std::map<const Derivation*,double>& m_p) {}
    virtual ScoreComponentCollection getRegularisationGradientFactor() {return ScoreComponentCollection();}
    virtual float getRegularisation() {return 0.0;}
    
  private:
    std::vector<ScoreComponentCollection> m_featureVectors;
    std::vector  <int>  m_lengths;
    std::vector <BleuSufficientStats> m_sufficientStats; 
    
    DerivationCollector m_derivationCollector;
    const int m_samples;
    ScoreComponentCollection getFeatureExpectations(const vector<double>& importanceWeights) const;
    int m_numSents;
    int GetNumSents() { return m_numSents;}
  protected:   
    void AggregateSuffStats(int);    
    
  };
  
}

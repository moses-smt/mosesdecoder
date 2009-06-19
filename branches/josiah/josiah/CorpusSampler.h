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
    :  m_samples(samples), ExpectedLossCollector() {
      sampler.AddCollector(&m_derivationCollector);
      m_featureVectors.resize(m_samples);
      m_lengths.resize(m_samples);
      m_sufficientStats.resize(m_samples);
    }
    virtual ~CorpusSamplerCollector() {}
    virtual void collect(Sample& sample);
    void resample(int);
    virtual float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len, float * unreg_exp_gain, float *scaling_gradient);
    virtual void UpdateHessianVProduct(ScoreComponentCollection* hessian, const ScoreComponentCollection& v) {}
 #ifdef MPI_ENABLED  
    void AggregateSamples(int);
#endif
    void reset();   
    float getReferenceLength();
    
  private:
    std::vector<ScoreComponentCollection> m_featureVectors;
    std::vector  <int>  m_lengths;
    std::vector <BleuSufficientStats> m_sufficientStats; 
    
    DerivationCollector m_derivationCollector;
    const int m_samples;
    ScoreComponentCollection getFeatureExpectations(const vector<double>& importanceWeights) const;

  };
  
}

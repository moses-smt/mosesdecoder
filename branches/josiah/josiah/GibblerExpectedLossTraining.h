#pragma once

#include <map>
#include <utility>

#include "Derivation.h"
#include "Gibbler.h"
#include "MpiDebug.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

using namespace Moses;

namespace Josiah {

class GainFunction;
//class Derivation;

class ExpectedLossCollector : public SampleCollector {
  public:
    ExpectedLossCollector(const GainFunction& f) :   g(f) {}
    virtual ~ExpectedLossCollector() {}
    virtual void collect(Sample& sample);
    // returns the expected gain and expected sentence length
    virtual float UpdateGradient(ScoreComponentCollection* gradient, float* exp_len, float* unreg_gain);
    virtual void UpdateHessianVProduct(ScoreComponentCollection* hessian, const ScoreComponentCollection& v);
    
    
  protected:
    ScoreComponentCollection getFeatureExpectations(const vector<double>& importanceWeights) const;
    /** Hooks for adding, eg, entropy regularisation. The first is added in to the gradient, the second to the objective.*/
    virtual float getRegularisationGradientFactor(size_t i) {return 0;}
    virtual float getRegularisation() {return 0;}
    virtual bool ComputeScaleGradient() {return false;}
  
  private:
    std::vector<ScoreComponentCollection> m_featureVectors;
    std::vector<float> m_gains;
    std::vector<size_t> m_lengths;
//    std::vector<Derivation> m_samples;
    const GainFunction& g;
    
  
};

}

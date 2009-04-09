#include "GibblerAnnealedExpectedLossTrainer.h"

#include "GainFunction.h"
#include "Hypothesis.h"
#include "Derivation.h"

using namespace std;

namespace Josiah {
  
float GibblerAnnealedExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient, float* exp_len) {
  //the distribution is fetched here so that it only has to be done once during gradient calculation
  m_p.clear();
  m_derivationCollector.getDistribution(m_p);
  return ExpectedLossCollector::UpdateGradient(gradient,exp_len);
}

float GibblerAnnealedExpectedLossCollector::getRegularisationGradientFactor(size_t i) {
  double temperature = GetTemperature();
  const Derivation* d = m_derivationCollector.getSample(i);
  float prob =  m_p[d];
  return -temperature * log (N()*prob) ;
}

float GibblerAnnealedExpectedLossCollector:: getRegularisation() {
  
  return GetTemperature() * m_derivationCollector.getEntropy();
}

  
  
  
/*float GibblerAnnealedExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient,
                                                             float *exp_len)  {
  ScoreComponentCollection feature_expectations = getFeatureExpectations();
  ScoreComponentCollection grad = feature_expectations; grad.ZeroAll();
  map<Derivation, float>::iterator si;
  
  double exp_gain = 0;
  double temperature = GetTemperature();
  
  
  for (si = m_gain.begin(); si != m_gain.end(); ++si) {
    const Derivation& derivation = si->first;
    ScoreComponentCollection d = derivation.getFeatureValues();
    const float gain = si->second;
    size_t count = m_counts[derivation];
    const float prob = static_cast<float>(count) / N() ;
    float entropy_factor = -temperature * (log (prob) + 1) ;
    d.MinusEquals(feature_expectations);
    d.MultiplyEquals(m_quenchTemp * prob * (gain+entropy_factor));
    exp_gain += gain * prob;
    grad.PlusEquals(d);
  }
  
  gradient->PlusEquals(grad);
  cerr << "Gradient: " <<  grad << endl;
  
  float entropy = ComputeEntropy();
  exp_gain += entropy * temperature;
  
  if (exp_len)
    *exp_len = static_cast<float>(tot_len) / N();
    
  return exp_gain;
}
  
float GibblerAnnealedExpectedLossCollector::ComputeEntropy() {
  map<Derivation, size_t>::const_iterator ci;    
  float entropy= 0;
  for (ci = m_counts.begin(); ci != m_counts.end(); ++ci) {
    float prob = static_cast<float>(ci->second) / N() ;
    entropy -= prob * log(prob);
  }
  cerr << "Entropy is " << entropy << endl; 
  if (entropy <0 || entropy > log(N() + 1e-2)) cerr << "entropy is negative or above upper bound, must be wrong; " << entropy << endl;
  return entropy;  
}  */
  

}


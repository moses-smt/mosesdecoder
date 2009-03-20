#include "GibblerAnnealedExpectedLossTrainer.h"

#include "GainFunction.h"
#include "Hypothesis.h"
#include "Derivation.h"

using namespace std;

namespace Josiah {
  
void GibblerAnnealedExpectedLossCollector::collect(Sample& s) {
  ++n;  // increment total samples seen
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const float gain = g.ComputeGain(trans);
  tot_len += trans.size();
  VERBOSE(2,"Gain " <<  gain << "\tFeatures=" << s.GetFeatureValues() << endl);
  feature_expectations.PlusEquals(s.GetFeatureValues());
  ++m_counts[Derivation(s)];
  m_gain[Derivation(s)] = gain;
}
  
float GibblerAnnealedExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient,
                                                             float *exp_len)  {

  feature_expectations.DivideEquals(n);
  ScoreComponentCollection grad = feature_expectations; grad.ZeroAll();
  map<Derivation, float>::iterator si;
  
  double exp_gain = 0;
  double temperature = GetTemperature();
  
  for (si = m_gain.begin(); si != m_gain.end(); ++si) {
    const Derivation& derivation = si->first;
    ScoreComponentCollection d = derivation.getFeatureValues();
    const float gain = si->second;
    size_t count = m_counts[derivation];
    const float prob = static_cast<float>(count) / n ;
    float entropy_factor = -temperature * (log (prob) + 1) ;
    d.MinusEquals(feature_expectations);
    d.MultiplyEquals(prob * (gain+entropy_factor));
    exp_gain += gain * prob;
    grad.PlusEquals(d);
    cerr << "Plain Expected gain " << exp_gain << endl;
  }
  
  gradient->PlusEquals(grad);
  
  float entropy = ComputeEntropy();
  exp_gain += entropy * temperature;
  
  if (exp_len)
    *exp_len = static_cast<float>(tot_len) / n;
    
  return exp_gain;
}
  
float GibblerAnnealedExpectedLossCollector::ComputeEntropy() {
  map<Derivation, size_t>::const_iterator ci;    
  float entropy= 0;
  for (ci = m_counts.begin(); ci != m_counts.end(); ++ci) {
    float prob = static_cast<float>(ci->second) / n ;
    entropy -= prob * log(prob);
  }
  cerr << "Entropy is " << entropy << endl; 
  if (entropy <0 || entropy > log(n + 1e-2)) cerr << "entropy is negative or above upper bound, must be wrong; " << entropy << endl;
  return entropy;  
}  
  

}


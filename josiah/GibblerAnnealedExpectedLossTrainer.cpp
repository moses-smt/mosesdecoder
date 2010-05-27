#include "GibblerAnnealedExpectedLossTrainer.h"

#include "GainFunction.h"
#include "Hypothesis.h"
#include "Derivation.h"

using namespace std;

namespace Josiah {
  
float GibblerAnnealedExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient, float* exp_len, float *unreg_exp_gain) {
  //the distribution is fetched here so that it only has to be done once during gradient calculation
  m_p.clear();
  m_derivationCollector.getDistribution(m_p);
  return ExpectedLossCollector::UpdateGradient(gradient,exp_len, unreg_exp_gain);
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

}


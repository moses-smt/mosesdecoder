#include "GibblerAnnealedExpectedLossTrainer.h"

#include "Hypothesis.h"
#include "Derivation.h"

using namespace std;

namespace Josiah {
  
float GibblerAnnealedExpectedLossCollector::UpdateGradient(FVector* gradient, FValue* exp_len, FValue *unreg_exp_gain) {
  //the distribution is fetched here so that it only has to be done once during gradient calculation
  m_p.clear();
  m_derivationCollector.getDistribution(m_p);
  return ExpectedLossCollector::UpdateGradient(gradient,exp_len, unreg_exp_gain);
}

float GibblerAnnealedExpectedLossCollector::getRegularisationGradientFactor(size_t i) {
  FValue temperature = GetTemperature();
  const Derivation* d = m_derivationCollector.getSample(i);
  FValue prob =  m_p[d];
  return -temperature * log (N()*prob) ;
}

float GibblerAnnealedExpectedLossCollector:: getRegularisation() {
  
  return GetTemperature() * m_derivationCollector.getEntropy();
}

}


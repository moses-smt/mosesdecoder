#include "GibblerExpectedLossTraining.h"

#include "GainFunction.h"
#include "Hypothesis.h"

using namespace std;

namespace Josiah {

void ExpectedLossCollector::collect(Sample& s) {
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const float gain = g.ComputeGain(trans);
  m_lengths.push_back(trans.size());
  VERBOSE(2, gain << "\tFeatures=" << s.GetFeatureValues() << endl);
  m_gains.push_back(gain);
  m_featureVectors.push_back(s.GetFeatureValues());
}

float ExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient,float *exp_len) {
  
  //retrieve importance weights
  vector<float> importanceWeights;
  getImportanceWeights(importanceWeights);
  
  ScoreComponentCollection feature_expectations = getFeatureExpectations(importanceWeights);
  
  //gradient computation
  ScoreComponentCollection grad;
  double exp_gain = 0;
  for (size_t i = 0; i < N(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    const float gain = m_gains[i];
    fv.MinusEquals(feature_expectations);
    fv.MultiplyEquals(gain + getRegularisationGradientFactor(i));
    exp_gain += gain*importanceWeights[i];
    fv.MultiplyEquals(importanceWeights[i]);
    grad.PlusEquals(fv);
  }
  
  exp_gain += getRegularisation();

  cerr << "Gradient: " << grad << endl;
  cerr << "Exp gain:  " << exp_gain << endl;
  
  gradient->PlusEquals(grad);
  //expected length
  if (exp_len) {
    exp_len = 0;
    for (size_t i = 0; i < N(); ++i) {
      *exp_len += importanceWeights[i] * m_lengths[i];
    }
  }

  return exp_gain;
}

ScoreComponentCollection ExpectedLossCollector::getFeatureExpectations(const vector<float>& importanceWeights) const {
  ScoreComponentCollection feature_expectations;
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    fv.MultiplyEquals(importanceWeights[i]);
    feature_expectations.PlusEquals(fv);
  }
  return feature_expectations;
}


}


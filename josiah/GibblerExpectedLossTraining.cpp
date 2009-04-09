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
  

  const vector<double>& importanceWeights =  getImportanceWeights();
  
  ScoreComponentCollection feature_expectations = getFeatureExpectations(importanceWeights);

  //cerr << "FEXP: " << feature_expectations << endl;
  
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

  cerr << "Gradient: " << grad << endl;
  cerr << "Exp gain without reg term :  " << exp_gain << endl;
  exp_gain += getRegularisation();
  cerr << "Exp gain with reg term:  " << exp_gain << endl;
  
  gradient->PlusEquals(grad);
  //expected length
  if (exp_len) {
    *exp_len = 0;
    for (size_t i = 0; i < N(); ++i) {
      *exp_len += importanceWeights[i] * m_lengths[i];
    }
  }

  return exp_gain;
}

  
void ExpectedLossCollector::UpdateHessianVProduct(ScoreComponentCollection* hessian, const ScoreComponentCollection& v) {
  const vector<double>& importanceWeights =  getImportanceWeights();
  ScoreComponentCollection feature_expectations = getFeatureExpectations(importanceWeights);

  float expectedVF = 0;
  ScoreComponentCollection expected_FVF = feature_expectations;
  expected_FVF.ZeroAll();
  
  //Calculate the expectation of v * f
  for (size_t i = 0; i < N(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    expectedVF += fv.InnerProduct(v);
  }
  
  //Calculate the expectation of f [vf  -E[vf]]
  for (size_t i = 0; i < N(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    float vf = fv.InnerProduct(v);
    vf -=expectedVF;
    fv.MultiplyEquals(vf);
    expected_FVF.PlusEquals(fv);   
  }
  
  //Now for the Hessian * v calc
  for (size_t i = 0; i < N(); ++i) {
    const ScoreComponentCollection& featureVector = m_featureVectors[i];
    const float gain = m_gains[i];
    ScoreComponentCollection fv = featureVector;
    fv.MinusEquals(feature_expectations);
    float vf = v.InnerProduct(featureVector);
    vf -= expectedVF;
    fv.MultiplyEquals(vf);
    fv.MinusEquals(expected_FVF);
    fv.MultiplyEquals(importanceWeights[i]);
    fv.MultiplyEquals(gain + getRegularisationGradientFactor(i));
    hessian->PlusEquals(fv);
  }

}  
  
ScoreComponentCollection ExpectedLossCollector::getFeatureExpectations(const vector<double>& importanceWeights) const {
  //do calculation at double precision to try to maintain accuracy
  vector<double> sum(StaticData::Instance().GetTotalScoreComponents());
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    for (size_t j = 0; j < fv.size(); ++j) {
        sum[j] += fv[j]*importanceWeights[i];
    }
    //cerr << "fexp: ";// << feature_expectations << endl;
    //copy(sum.begin(),sum.end(),ostream_iterator<double>(cerr," "));
    //cerr << endl;
  }
  vector<float> truncatedSum(sum.size());
  for (size_t i = 0; i < sum.size(); ++i) {
      truncatedSum[i] = static_cast<float>(sum[i]);
  }
  return ScoreComponentCollection(truncatedSum);
}


}


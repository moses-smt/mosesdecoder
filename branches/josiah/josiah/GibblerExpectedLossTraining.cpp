#include "GibblerExpectedLossTraining.h"

#include "GainFunction.h"
#include "Hypothesis.h"
#include "Decoder.h"

using namespace std;

namespace Josiah {

void ExpectedLossCollector::collect(Sample& s) {
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const float gain = g.ComputeGain(trans);
  m_lengths.push_back(trans.size());
  VERBOSE(2, gain << "\tFeatures=" << s.GetFeatureValues() << endl);
  //VERBOSE(0, "Collected : Target " << s << ", gain " << gain << "\tFeatures=" << s.GetFeatureValues() << endl);
  m_gains.push_back(gain);
//  m_samples.push_back(Derivation(s));
  m_featureVectors.push_back(s.GetFeatureValues());
  MPI_VERBOSE(2,"Sample: " << Derivation(s) << endl) 
}

float ExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient,float *exp_len, float *unreg_exp_gain) {
  

  const vector<double>& importanceWeights =  getImportanceWeights();
  
  ScoreComponentCollection feature_expectations = getFeatureExpectations(importanceWeights);

  MPI_VERBOSE(1,"FEXP: " << feature_expectations << endl)
  
  vector<float> w;
  GetFeatureWeights(&w);
  float exp_score = feature_expectations.InnerProduct(w);
  float scaling_gradient = 0.0;
  
  //gradient computation
  ScoreComponentCollection grad(gradient->size());
  double exp_gain = 0;
  for (size_t i = 0; i < N(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    MPI_VERBOSE(2,"FV: " << fv)
    const float gain = m_gains[i];
    fv.MinusEquals(feature_expectations);
    MPI_VERBOSE(2,"DIFF: " << fv)
    fv.MultiplyEquals(gain + getRegularisationGradientFactor(i));
    MPI_VERBOSE(2,"GAIN: " << gain << " RF: " << getRegularisationGradientFactor(i) << " IF: " << importanceWeights[i] << endl)
    exp_gain += gain*importanceWeights[i];
    //VERBOSE(0, "Sample=" << m_samples[i] << ", gain: " << gain << ", imp weights: " << importanceWeights[i] << endl);
    fv.MultiplyEquals(importanceWeights[i]);
    MPI_VERBOSE(2,"WEIGHTED: " << fv << endl)
    grad.PlusEquals(fv);
    MPI_VERBOSE(2,"grad: " << grad << endl)
    if (ComputeScaleGradient()) {
      scaling_gradient +=  (gain + getRegularisationGradientFactor(i)) * (m_featureVectors[i].InnerProduct(w) - exp_score) * importanceWeights[i] ;
    }
      
  }
  MPI_VERBOSE(1,"Gradient: " << grad << endl)

  cerr << "Gradient: " << grad << endl;
  cerr << "Exp gain without reg term :  " << exp_gain << endl;
  *unreg_exp_gain = exp_gain;
  exp_gain += getRegularisation();
  cerr << "Exp gain with reg term:  " << exp_gain << endl;
  
  if (ComputeScaleGradient()) {
    cerr << "Scaling gradient:  " << scaling_gradient << endl;
    vector<float> scaling_gradient_vec(gradient->size());
    scaling_gradient_vec[gradient->size()-1] = scaling_gradient;
    ScoreComponentCollection sc(scaling_gradient_vec);
    grad.PlusEquals(sc);
  }
  
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
    fv.MultiplyEquals(importanceWeights[i]);
    expectedVF += fv.InnerProduct(v);
  }
  
  //Calculate the expectation of f [vf  -E[vf]]
  for (size_t i = 0; i < N(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    float vf = fv.InnerProduct(v);
    vf -=expectedVF;
    fv.MultiplyEquals(vf);
    fv.MultiplyEquals(importanceWeights[i]);
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


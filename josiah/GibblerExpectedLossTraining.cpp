#include "GibblerExpectedLossTraining.h"

#include "GainFunction.h"
#include "Hypothesis.h"
#include "Decoder.h"
#include <ext/algorithm>

using namespace std;
using namespace __gnu_cxx;

namespace Josiah {

void ExpectedLossCollector::collect(Sample& s) {
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const float gain = g[0]->ComputeGain(trans);
  m_lengths.push_back(trans.size());
  ScoreComponentCollection fvs = s.GetFeatureValues();
  ScoreComponentCollection rbFvs = s.GetConditionalFeatureValues();
  VERBOSE(2, gain << "\tFeatures=" << fvs << endl);
  VERBOSE(2, gain << "\tRao-Blackwellised features=" << rbFvs << endl);
  //VERBOSE(0, "Collected : Target " << s << ", gain " << gain << "\tFeatures=" << s.GetFeatureValues() << endl);
  m_gains.push_back(gain);
//  m_samples.push_back(Derivation(s));
  m_featureVectors.push_back(fvs);
  m_rbFeatureVectors.push_back(rbFvs);
  MPI_VERBOSE(2,"Sample: " << Derivation(s) << endl) 
}

float ExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient,float *exp_len, float *unreg_exp_gain) {
  

  
  ScoreComponentCollection feature_expectations = getFeatureExpectations();

  MPI_VERBOSE(1,"FEXP: " << feature_expectations << endl)
  
  vector<float> w;
  GetFeatureWeights(&w);
  float exp_score = feature_expectations.InnerProduct(w);
  
  //gradient computation
  ScoreComponentCollection grad;
  double exp_gain = 0;
  for (size_t i = 0; i < N(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    MPI_VERBOSE(2,"FV: " << fv)
    const float gain = m_gains[i];
    fv.MinusEquals(feature_expectations);
    MPI_VERBOSE(2,"DIFF: " << fv)
    fv.MultiplyEquals(gain + getRegularisationGradientFactor(i));
    MPI_VERBOSE(2,"GAIN: " << gain << " RF: " << getRegularisationGradientFactor(i) <<  endl);
    exp_gain += gain/N();
    fv.DivideEquals(N());
    MPI_VERBOSE(2,"WEIGHTED: " << fv << endl)
    grad.PlusEquals(fv);
    MPI_VERBOSE(2,"grad: " << grad << endl)
      
  }
  cerr << "Exp gain without reg term :  " << exp_gain << endl;
  *unreg_exp_gain = exp_gain;
  exp_gain += getRegularisation();
  cerr << "Exp gain with reg term:  " << exp_gain << endl;
  
  gradient->PlusEquals(grad);
  MPI_VERBOSE(1,"Gradient: " << grad << endl)

  cerr << "Gradient: " << grad << endl;
  
  //expected length
  if (exp_len) {
    *exp_len = 0;
    for (size_t i = 0; i < N(); ++i) {
        *exp_len += m_lengths[i];
    }
    *exp_len /= N();
  }

  return exp_gain;
}



 

double ExpectedLossCollector::getExpectedGain() const {
    double exp_gain = 0;
    for (size_t i = 0; i < N(); ++i) {
        exp_gain += m_gains[i];
    }
    exp_gain /= N();
    return exp_gain;
}
  
ScoreComponentCollection ExpectedLossCollector::getFeatureExpectations() const {
  //do calculation at double precision to try to maintain accuracy
  vector<double> sum(StaticData::Instance().GetTotalScoreComponents());
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    ScoreComponentCollection fv = m_rbFeatureVectors[i];
    for (size_t j = 0; j < fv.size(); ++j) {
        sum[j] += fv[j]/m_featureVectors.size();
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

ScoreComponentCollection ExactExpectedLossCollector::getFeatureExpectations() const {
  IFVERBOSE(2) { 
    cerr << "distribution: ";
    copy(m_exactProbs.begin(),m_exactProbs.end(),ostream_iterator<double>(cerr," "));
    cerr << endl;
  }
  //do calculation at double precision to try to maintain accuracy
  vector<double> sum(StaticData::Instance().GetTotalScoreComponents());
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    for (size_t j = 0; j < fv.size(); ++j) {
        sum[j] += fv[j]* m_exactProbs[i];
    }
  }
  vector<float> truncatedSum(sum.size());
  for (size_t i = 0; i < sum.size(); ++i) {
      truncatedSum[i] = static_cast<float>(sum[i]);
  }
  return ScoreComponentCollection(truncatedSum);
}

void ExactExpectedLossCollector::ShrinkAndCalcTrueDistribution() {
  //Now shrink
    size_t newSize = (size_t)(N() * m_shrinkFactor);
  if (m_randomShrink) {
    ShrinkRandom(newSize);  
  }
  else {
    ShrinkByProb(newSize);
  }
  SetN(newSize);
}

struct ProbGreaterThan :  public std::binary_function<const pair<const size_t,double>&,const pair<const size_t,double>&,bool>{
    bool operator()(const pair<const size_t,double>& d1, const pair<const size_t,double>& d2) const {
      return d1.second > d2.second; 
    }
};

void ExactExpectedLossCollector::ShrinkByProb(size_t newSize) {
  vector<float> w;
  GetFeatureWeights(&w);
  
  //True Derivation Probs
  vector< pair<size_t, double> > exactProbs;
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    float score = m_featureVectors[i].InnerProduct(w);
    exactProbs.push_back(make_pair(i, score));
  }
  
  ProbGreaterThan comparator;
  nth_element(exactProbs.begin(), exactProbs.begin() + newSize, exactProbs.end(), comparator);
  
  IFVERBOSE(2) {
    cerr << "retained derivations: ";
    for (size_t i = 0; i < newSize; ++i) {
      cerr << exactProbs[i].second << " ";
    }
    cerr << endl;
  
    cerr << "discarded derivations: ";
    for (size_t i = newSize; i < N(); ++i) {
     cerr << exactProbs[i].second << " ";
    }
    cerr << endl;
  }
  
  vector<double> bestProbs;
  std::vector<ScoreComponentCollection> bestFeatureVectors;
  std::vector<float> bestGains;
  std::vector<size_t> bestLengths;
  for (size_t i = 0; i < newSize; ++i) {
    bestProbs.push_back(1.0/newSize); 
    bestFeatureVectors.push_back(m_featureVectors[exactProbs[i].first]);
    bestGains.push_back(m_gains[exactProbs[i].first]);
    bestLengths.push_back(m_lengths[exactProbs[i].first]);
  }
  m_exactProbs.swap(bestProbs);
  m_featureVectors.swap(bestFeatureVectors);
  m_gains.swap(bestGains);
  m_lengths.swap(bestLengths);
}

void ExactExpectedLossCollector::ShrinkRandom(size_t newSize) {
  vector<size_t> indices;
  for (size_t i = 0; i < N(); ++i) {
    indices.push_back(i);
  }
  vector<size_t> randomIndices(newSize);
  random_sample(indices.begin(), indices.end(), randomIndices.begin(), randomIndices.end());
  
  
  vector<float> w;
  GetFeatureWeights(&w);
  
  IFVERBOSE(2) {
  cerr << "retained derivations: ";
  for (size_t i = 0; i < newSize; ++i) {
   cerr << m_featureVectors[randomIndices[i]].InnerProduct(w) << " ";
  }
  cerr << endl;
  }
  
  vector<double> randomProbs;
  std::vector<ScoreComponentCollection> featureVectors;
  std::vector<float> gains;
  std::vector<size_t> lengths;
  for (size_t i = 0; i < randomIndices.size(); ++i) {
    randomProbs.push_back(1.0/newSize);
    featureVectors.push_back(m_featureVectors[randomIndices[i]]);
    gains.push_back(m_gains[randomIndices[i]]);
    lengths.push_back(m_lengths[randomIndices[i]]);
  }
  m_exactProbs.swap(randomProbs);
  m_featureVectors.swap(featureVectors);
  m_gains.swap(gains);
  m_lengths.swap(lengths);
}

float ExactExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient,float *exp_len, float *unreg_exp_gain) {

  ShrinkAndCalcTrueDistribution();

  ScoreComponentCollection feature_expectations = getFeatureExpectations(); //


  MPI_VERBOSE(2,"FEXP: " << feature_expectations << endl)
  
  vector<float> w;
  GetFeatureWeights(&w);
  float exp_score = feature_expectations.InnerProduct(w);
  
  //gradient computation
  ScoreComponentCollection grad;
  double exp_gain = 0;
  for (size_t i = 0; i < N(); ++i) {
    ScoreComponentCollection fv = m_featureVectors[i];
    MPI_VERBOSE(2,"FV: " << fv)
    const float gain = m_gains[i];
    fv.MinusEquals(feature_expectations);
    MPI_VERBOSE(2,"DIFF: " << fv)
    fv.MultiplyEquals(gain + getRegularisationGradientFactor(i));
    MPI_VERBOSE(2,"GAIN: " << gain << " RF: " << getRegularisationGradientFactor(i) << " IF: " << m_exactProbs[i] << endl)
    exp_gain += gain*m_exactProbs[i];
    fv.MultiplyEquals(m_exactProbs[i]);
    MPI_VERBOSE(2,"WEIGHTED: " << fv << endl)
    grad.PlusEquals(fv);
    MPI_VERBOSE(2,"grad: " << grad << endl)
      
  }
  cerr << "Exp gain without reg term :  " << exp_gain << endl;
  *unreg_exp_gain = exp_gain;
  exp_gain += getRegularisation();
  cerr << "Exp gain with reg term:  " << exp_gain << endl;
  
  gradient->PlusEquals(grad);
  MPI_VERBOSE(1,"Gradient: " << grad << endl)

  cerr << "Gradient: " << grad << endl;
  
  //expected length
  if (exp_len) {
    *exp_len = 0;
    for (size_t i = 0; i < N(); ++i) {
      *exp_len += m_exactProbs[i] * m_lengths[i];
    }
  }

  return exp_gain;  
}
  

}


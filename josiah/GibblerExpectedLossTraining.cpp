#include <ext/algorithm>

#include "GibblerExpectedLossTraining.h"

#include "Hypothesis.h"
#include "WeightManager.h"


using namespace std;
using namespace __gnu_cxx;

namespace Josiah {

void ExpectedLossCollector::collect(Sample& s) {
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const FValue gain = m_gainFunction->Evaluate(trans);
  m_lengths.push_back(trans.size());
  const FVector& fvs = s.GetFeatureValues();
  const FVector& rbFvs = s.GetConditionalFeatureValues();
  VERBOSE(2, gain << "\tFeatures=" << fvs << endl);
  VERBOSE(2, gain << "\tRao-Blackwellised features=" << rbFvs << endl);
  //VERBOSE(0, "Collected : Target " << s << ", gain " << gain << "\tFeatures=" << s.GetFeatureValues() << endl);
  m_gains.push_back(gain);
//  m_samples.push_back(Derivation(s));
  m_featureVectors.push_back(fvs);
  m_rbFeatureVectors.push_back(rbFvs);
  MPI_VERBOSE(2,"Sample: " << Derivation(s) << endl) 
}

float ExpectedLossCollector::UpdateGradient(FVector* gradient,FValue *exp_len, FValue *unreg_exp_gain) {
  

  
  FVector feature_expectations = getFeatureExpectations();

  MPI_VERBOSE(1,"FEXP: " << feature_expectations << endl)
  
  const FVector& weights = WeightManager::instance().get();
  FValue exp_score = inner_product(feature_expectations, weights);
  
  //gradient computation
  FVector grad;
  FValue exp_gain = 0;
  for (size_t i = 0; i < N(); ++i) {
    FVector fv = m_featureVectors[i];
    MPI_VERBOSE(2,"FV: " << fv)
    const FValue gain = m_gains[i];
    fv -= feature_expectations;
    MPI_VERBOSE(2,"DIFF: " << fv)
    fv *= (gain + getRegularisationGradientFactor(i));
    MPI_VERBOSE(2,"GAIN: " << gain << " RF: " << getRegularisationGradientFactor(i) <<  endl);
    exp_gain += gain/N();
    fv /= N();
    MPI_VERBOSE(2,"WEIGHTED: " << fv << endl)
    grad += fv;
    MPI_VERBOSE(2,"grad: " << grad << endl)
      
  }
  cerr << "Exp gain without reg term :  " << exp_gain << endl;
  *unreg_exp_gain = exp_gain;
  exp_gain += getRegularisation();
  cerr << "Exp gain with reg term:  " << exp_gain << endl;
  
  (*gradient) += grad;
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
  
FVector ExpectedLossCollector::getFeatureExpectations() const {
  FVector sum;
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    sum += m_featureVectors[i];
  }
  sum /= m_featureVectors.size();
    
  return sum;
}






}


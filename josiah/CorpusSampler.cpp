#include "CorpusSampler.h"

#include "Decoder.h"
#include "Hypothesis.h"
#include "GibbsOperator.h"

#ifdef MPI_ENABLED
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/serialization/vector.hpp>
namespace mpi = boost::mpi;
#endif

using namespace std;

namespace Josiah {
  
void CorpusSamplerCollector::collect(Sample& s) {
   //nothing to do  
}

//Resample based on derivation distribution
void CorpusSamplerCollector::resample(int sent) {
  std::map<const Derivation*,double> m_p, m_resampled_p;
  m_derivationCollector.getDistribution(m_p); //fetch the distribution
    
  //copy it to a vector, will be easier for further processing
  vector<const Derivation*> derivations;
  vector<double> scores;
    
  for (map<const Derivation*,double>::iterator it = m_p.begin(); it != m_p.end(); ++it) {
    derivations.push_back(it->first);
    scores.push_back(it->second);
  }
  
  //Printing out distribution

  IFVERBOSE(2) {
    for (size_t i = 0; i < derivations.size();++i) {
      cerr << *derivations[i] << " has score " << scores[i] <<endl;
    }  
  }
  
  
  double sum = scores[0];
  for (size_t i = 1; i < scores.size(); ++i) {
    sum = log_sum(sum,scores[i]);
  }
  
  transform(scores.begin(),scores.end(),scores.begin(),bind2nd(minus<double>(),sum));
  
  
  //now sample from this
  for (int j = 0; j < m_samples; ++j) {
    
    //random number between 0 and 1
    double random =  RandomNumberGenerator::instance().next();//(double)rand() / RAND_MAX;
      
    random = log(random);
      
    //now figure out which sample
    size_t position = 1;
    sum = scores[0];
    for (; position < scores.size() && sum < random; ++position) {
      sum = log_sum(sum,scores[position]);
    }
      
    size_t chosen =  position-1;  
    MPI_VERBOSE(2, "Chosen derivation " << chosen << endl)
      
    //Store chosen derivation's feature values and length
    const Derivation* chosenDeriv = derivations[chosen]; 
    m_resampled_p[chosenDeriv] += 1.0/m_samples;
    MPI_VERBOSE(2, "Chosen deriv " << *chosenDeriv << endl)
    MPI_VERBOSE(2, "Chosen deriv size" << chosenDeriv->getTargetSentenceSize() << endl)
    
    m_featureVectors.at(j) += chosenDeriv->getFeatureValues();
    MPI_VERBOSE(2, "Feature vector : " << m_featureVectors.at(j) << endl)
    m_lengths[j] += chosenDeriv->getTargetSentenceSize();
    MPI_VERBOSE(2, "Lengths : " << m_lengths.at(j) << endl)
      
    //Store chosen derivation's gain sufficient stats
    SufficientStats *stats = new BleuSufficientStats(4);
    
    std::vector<const Factor*> yield;
    chosenDeriv->getTargetFactors(yield);
    g[sent]->GetSufficientStats(yield, stats);
    
    m_sufficientStats[j] += *(static_cast<BleuSufficientStats*>(stats));
    MPI_VERBOSE(2, "Stats : " << m_sufficientStats.at(j) << endl)
    delete stats;
  }    
  
  IFVERBOSE(2) {
  cerr << "After resampling, distribution is : " << endl;
  for (map<const Derivation*,double>::iterator it = m_resampled_p.begin(); it != m_resampled_p.end(); ++it) {
    cerr << *(it->first) << "has score " << it->second << endl;
  }
  }
  
  setRegularisation(m_p);
  setRegularisationGradientFactor(m_p);
  
  //Now reset the derivation collector
  m_derivationCollector.reset();
  m_numSents++;
  
}

  
  
#ifdef MPI_ENABLED    

template<class T>
struct VectorPlus {
    vector<T> operator()(const vector<T>& lhs, const vector<T>& rhs) const {
        assert(lhs.size() == rhs.size());
        vector<T> sum(lhs.size());
        for (size_t i = 0; i < lhs.size(); ++i) sum.push_back(lhs[i] + rhs[i]);
        return sum;
    }
};
  
void CorpusSamplerCollector::AggregateSamples(int rank) {
  AggregateSuffStats(rank);
}
   
void CorpusSamplerCollector::AggregateSuffStats(int rank) {
  /*what do we need to store?
  1. Feature Vectors
  2. Lengths
  3. Bleu Stats 
  */
  vector  <int>  lengths (m_lengths.size());
  FVector featsVecs, recFeatsVecs;
  FVector suffStats, recSuffStats;
  int numSents;

  mpi::communicator world;
  
  //Reduce length
  mpi::reduce(world,m_lengths,lengths,VectorPlus<int>(),0);
//  if (MPI_SUCCESS != MPI_Reduce(const_cast<int*>(&m_lengths[0]), &lengths[0], m_lengths.size(), MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);

  //Reduce numSents
//  if (MPI_SUCCESS != MPI_Reduce(&m_numSents, &numSents, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
  mpi::reduce(world,m_numSents,numSents,std::plus<int>(),0);
  
  //Reduce feature vectors and sufficient stats
  mpi::reduce(world,m_featureVectors,m_featureVectors,VectorPlus<FVector>(),0);
  mpi::reduce(world,m_sufficientStats,m_sufficientStats,VectorPlus<BleuSufficientStats>(),0);


  //MPI can't handle vector of vectors, so first concatenate elements together
  
  /*
  //Concatenate feature vectors
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    for (size_t j = 0; j < m_featureVectors[i].size(); ++j) {
      featsVecs.push_back(m_featureVectors[i][j]);
    }
  }
  
  //Concatenate sufficient stats 
  for (size_t i = 0; i < m_sufficientStats.size(); ++i) {
    vector < float > bleuStats = m_sufficientStats[i].data();
    for (size_t j = 0; j < bleuStats.size(); ++j) {
      suffStats.push_back(bleuStats[j]);
    }
  }
 
  if (rank == 0) {
    recFeatsVecs.resize(featsVecs.size());
    recSuffStats.resize(suffStats.size());
  }

  //Reduce FVs and SStats
  if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&featsVecs[0]), &recFeatsVecs[0], featsVecs.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
  if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&suffStats[0]), &recSuffStats[0], suffStats.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);

  
  //Unpack FVs and SStats, 
  if (rank == 0 ) {  
    //FVs
    size_t numFeats = recFeatsVecs.size() /  m_featureVectors.size();
    m_featureVectors.clear();

    for (size_t i = 0; i  < recFeatsVecs.size(); i += numFeats) {
      vector<float> features(recFeatsVecs.begin() + i, recFeatsVecs.begin() + i + numFeats);
      ScoreComponentCollection feats(features); 
      m_featureVectors.push_back(feats);   
    }

    //Suff Stats
    size_t sizeStats = recSuffStats.size() /  m_sufficientStats.size();
    m_sufficientStats.clear();
    
    for (size_t i = 0; i  < recSuffStats.size(); i += sizeStats) {
      vector<float> _stats(recSuffStats.begin() + i, recSuffStats.begin() + i + sizeStats);
      BleuSufficientStats stats(_stats);
      m_sufficientStats.push_back(stats);
    }
    */
    
    //Transfer lengths back
  if (rank == 0) {
    m_lengths = lengths;
    m_numSents = numSents;
  }
}
#endif
  
float CorpusSamplerCollector::UpdateGradient(FVector* gradient,FValue *exp_len, FValue *unreg_exp_gain) {
  FVector feature_expectations = getFeatureExpectations();
    
  MPI_VERBOSE(1,"FEXP: " << feature_expectations << endl)
    
  //gradient computation
  FVector grad;
  FValue exp_gain = 0;
  FValue gain = 0.0;
  for (size_t i = 0; i < m_featureVectors.size() ; ++i) {
    FVector fv = m_featureVectors[i];
    MPI_VERBOSE(2,"FV: " << fv)
    gain = SentenceBLEU::CalcBleu(m_sufficientStats[i], false);
    fv -= feature_expectations;
    MPI_VERBOSE(2,"DIFF: " << fv)
    fv *= gain;
    MPI_VERBOSE(2,"GAIN: " << gain  << endl);
    exp_gain += gain;
    grad += fv;
    MPI_VERBOSE(2,"grad: " << grad << endl);
  }
  grad /= m_featureVectors.size();
  exp_gain /= m_featureVectors.size();
  
  cerr << "Gradient without reg " << grad << endl;
  FVector regularizationGrad = getRegularisationGradientFactor();
  regularizationGrad /= GetNumSents();
  grad += regularizationGrad;
  
  
  cerr << "Exp gain without reg term :  " << exp_gain << endl;
  *unreg_exp_gain = exp_gain;
  exp_gain += getRegularisation()/GetNumSents();
  cerr << "Exp gain with reg term:  " << exp_gain << endl;
  
  *gradient += grad;
  MPI_VERBOSE(1,"Gradient: " << grad << endl)
  
  cerr << "Gradient: " << grad << endl;
  
  //expected length
 if (exp_len) {
    *exp_len = 0;
    for (size_t j = 0; j < m_sufficientStats.size(); ++j) {
      *exp_len += m_sufficientStats[j].hyp_len;
    }
  } 
  return exp_gain;
}
  
FVector CorpusSamplerCollector::getFeatureExpectations() const {
  FVector sum;
  for (size_t i = 0; i < m_featureVectors.size(); ++i) {
    sum += m_featureVectors[i];
  }
  return sum;
}
  
void CorpusSamplerCollector::reset() {
  m_featureVectors.clear(); m_featureVectors.resize(m_samples);
  m_lengths.clear(); m_lengths.resize(m_samples);
  m_sufficientStats.clear(); m_sufficientStats.resize(m_samples);
  m_numSents = 0;
}
  
float CorpusSamplerCollector::getReferenceLength() {
  float refLen(0.0); 
  for (size_t j = 0; j < m_sufficientStats.size(); ++j) {
    refLen += m_sufficientStats[j].ref_len;
  }
  return refLen;
}
  
}

#include "Sampler.h"
#include "StaticData.h"
#include "GibbsOperator.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "Gibbler.h"
#include "SampleAcceptor.h"
#include "SampleCollector.h"
#include "StopStrategy.h"
#include "Derivation.h"

namespace Josiah {
  void Sampler::AddOperator(GibbsOperator* o) {
    o->SetSampler(this); 
    m_operators.push_back(o); 
  }
  
  void Sampler::UpdateGainFunctionStats(const BleuSufficientStats& stats){
    m_suffStats += stats;
    m_numSuffStats++;
  }
  
  BleuSufficientStats* Sampler::GetSentenceGainFunctionStats(){
    m_averageSuffStats = m_suffStats / m_numSuffStats;
    return &m_averageSuffStats;  
  }
  
  void Sampler::ResetGainStats() {
    m_suffStats.Zero();
    m_numSuffStats = 0;
    m_optimalGain = 0;
    m_GainOptimalSol.ZeroAll();
  }
  
  void Sampler::SetOptimalGainSol(TranslationDelta* chosen, TranslationDelta* noChangeDelta) {
    m_GainOptimalSol = chosen->getSample().GetFeatureValues();
    m_GainOptimalSol.PlusEquals(chosen->getScores());
    m_GainOptimalSol.MinusEquals(noChangeDelta->getScores());
    
  }
  
  //void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const Josiah::feature_vector& extra_fv, SampleAcceptor* acceptor, bool collectAllSamples, bool defaultCtrIncrementer) {
//    Sample sample(starting,source,extra_fv);
//    ResetGainStats();
//    bool f = false;
//    
//    for (size_t j = 0; j < m_operators.size(); ++j) {
//      m_operators[j]->addSampleAcceptor(acceptor); 
//    }
//    
//    for (size_t k = 0; k < m_reheatings; ++k) {
//      if (m_burninIts) {
//        for (size_t j = 0; j < m_operators.size(); ++j) {
//          m_operators[j]->disableGainFunction(); // do not compute gains during burn-in
//        } 
//      }
//      for (size_t i = 0; i < m_burninIts; ++i) {
//        if ((i+1) % 5 == 0) { VERBOSE(1,'.'); f=true;}
//        if ((i+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
//        VERBOSE(2,"Gibbs burnin iteration: " << i << endl);
//        for (size_t j = 0; j < m_operators.size(); ++j) {
//          VERBOSE(2,"Sampling with operator " << m_operators[j]->name() << endl);
//          m_operators[j]->resetIterator(); 
//          if (m_as)
//            m_operators[j]->SetAnnealingTemperature(m_as->GetTemperatureAtTime(i));
//          
//          while (m_operators[j]->keepGoing() && i < m_burninIts) {
//            m_operators[j]->scan(sample,*options);  
//          }
//        }
//      }
//      if (f) VERBOSE(1,endl);
//      if (m_as) {
//        for (size_t j = 0; j < m_operators.size(); ++j)
//          m_operators[j]->Quench();
//      }
//      
//      for (size_t j = 0; j < m_operators.size(); ++j) {
//        m_operators[j]->addSampleAcceptor(acceptor);
//        m_operators[j]->enableGainFunction();
//        m_operators[j]->resetIterator(); 
//      }
//      
//      
//      size_t i = 0;
//      bool keepGoing = true;
//      while(keepGoing) {
//        if ((i+1) % 5 == 0) { VERBOSE(1,'.'); f=true; }
//        if ((i+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
//        VERBOSE(2,"Gibbs sampling iteration: " << i << endl);
//        for (size_t j = 0; j < m_operators.size() && keepGoing; ++j) {
//          m_operators[j]->resetIterator(); 
//          VERBOSE(2,"Sampling with operator " << m_operators[j]->name() << endl);
//          while (m_operators[j]->keepGoing() && keepGoing) {
//            m_operators[j]->scan(sample,*options); 
//          }
//          if (collectAllSamples) {
//              collectSample(sample);
//              if (!defaultCtrIncrementer)
//                ++i;
//              if (m_stopper->ShouldStop(i)) {
//                keepGoing = false;
//                break;
//              }
//          }
//        }
//        if (!collectAllSamples) {
//          collectSample(sample);
//        }  
//        if (defaultCtrIncrementer) {
//          ++i;
//        }
//        if (m_stopper->ShouldStop(i)) {
//          keepGoing = false;
//          break;
//        }
//        if (collectAllSamples && m_collectors[0]->N() > SAMPLEMAX) {
//          keepGoing = false;
//          break;
//        }
//          
//      }
//      if (f) VERBOSE(1,endl);
//    }
//
//  }
  
  void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const Josiah::feature_vector& extra_fv, SampleAcceptor* acceptor, bool collectAllSamples, bool defaultCtrIncrementer) {
    //The starting point for the chains
    Sample sample(starting,source,extra_fv);
    vector<Sample> newSamples(m_numChains, sample);
    vector<Sample*> samples(m_numChains);
    for (size_t i = 0; i < m_numChains; ++i) {
      samples[i] = &newSamples[i];
    }
    
    ResetGainStats();
    bool f = false;
    
    for (size_t j = 0; j < m_operators.size(); ++j) {
      m_operators[j]->addSampleAcceptor(acceptor); 
    }
    
    //Let's start with some burn-in
    if (m_burninIts) {
      for (size_t j = 0; j < m_operators.size(); ++j) {
          m_operators[j]->disableGainFunction(); // do not compute gains during burn-in
      } 
    }
    
    for (size_t i = 0; i < m_burninIts; ++i) {
      if ((i+1) % 5 == 0) { VERBOSE(1,'.'); f=true;}
      if ((i+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
      VERBOSE(2,"Gibbs burnin iteration: " << i << endl);
      for (size_t j = 0; j < m_operators.size(); ++j) {
        for (size_t k = 0; k < m_numChains; ++k) {
          VERBOSE(2,"Sampling chain " << k << " with operator " << m_operators[j]->name() << endl);
          m_operators[j]->resetIterator(); 
          if (m_as)
            m_operators[j]->SetAnnealingTemperature(m_as->GetTemperatureAtTime(i));
          
          while (m_operators[j]->keepGoing() && i < m_burninIts) {
            //for (size_t l = 0; l < m_numChains; ++l) {    
//              cerr << "Sample " << l << " = " << Derivation(newSamples[l]) << endl;
//            }  
            m_operators[j]->scan(*(samples[k]),*options);  
          }
        }  
      }
    }
    
    if (f) VERBOSE(1,endl);
    if (m_as) {
      for (size_t j = 0; j < m_operators.size(); ++j)
        m_operators[j]->Quench();
    }
      
    for (size_t j = 0; j < m_operators.size(); ++j) {
      m_operators[j]->addSampleAcceptor(acceptor);
      m_operators[j]->enableGainFunction();
      m_operators[j]->resetIterator(); 
    }
    
     
    
    //The actual chain    
    size_t i = 0;
    bool keepGoing = true;
    while(keepGoing) {
      if ((i+1) % 5 == 0) { VERBOSE(1,'.'); f=true; }
      if ((i+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
      VERBOSE(2,"Gibbs sampling iteration: " << i << endl);
      for (size_t j = 0; j < m_operators.size() && keepGoing; ++j) {
        for (size_t k = 0; k < m_numChains; ++k) {
          m_operators[j]->resetIterator(); 
          m_operators[j]->SetAnnealingTemperature(m_temperingSchedule[k]); 
          VERBOSE(2,"Sampling chain " << k << " with operator " << m_operators[j]->name() << endl);
          while (m_operators[j]->keepGoing() && keepGoing) {
            m_operators[j]->scan(*(samples[k]),*options); 
          }  
          //for (size_t l = 0; l < m_numChains; ++l) {    
//            cerr << "Sample " << l << " = " << Derivation(newSamples[l]) << endl;
//          }
        }
        //run exchange algorithm here
        IFVERBOSE(2) {
          for (size_t l = 0; l < m_numChains; ++l) {    
            cerr << "Before exchange, Sample " << l << " = " << Derivation(*(samples[l])) << endl;
          }  
        }
        
        if (m_numChains > 1) {
          exchangeSamples(samples, m_temperingSchedule);  
        }
        IFVERBOSE(2) {
          for (size_t l = 0; l < m_numChains; ++l) {    
            cerr << "After exchange, Sample " << l << " = " << Derivation(*(samples[l])) << endl;
          }  
        }
        
        collectSample(*samples[0]); //now collect
        ++i;
        if (m_stopper->ShouldStop(i)) {
          keepGoing = false;
          break;
        }
      }
      if (m_stopper->ShouldStop(i)) {
        keepGoing = false;
        break;
      }
      if (m_collectors.size() && m_collectors[0]->N() > SAMPLEMAX) {
        keepGoing = false;
        break;
      }
    }
    if (f) VERBOSE(1,endl);
  }  
  
  void Sampler::exchangeSamples(vector<Sample*>& samples, const vector<float>& temperatures) {
    size_t numChains = samples.size();   
    double random =  RandomNumberGenerator::instance().next();
    
    if (random > m_exchangeProb) {
      VERBOSE(2, "Random " << random << " > exchange Prob " << m_exchangeProb << ", not exchanging " <<endl)
      return;
    }
      
    random =  RandomNumberGenerator::instance().next();
    size_t numNeighbouringPairs = numChains - 1;
    
    
    double start = 0.0;
    double step = (1.0/numNeighbouringPairs);
    double next = start + step;
    size_t index;
    for (index = 0; index < numNeighbouringPairs; ++index) {
      if (random < next)
        break;
      start = next;
      next = start+step;
    }
    
    //try to exchange index and index +1
    VERBOSE(2, "Trying to exchange " << index << " and " << index +1 << endl)
    Sample* sample_index = samples[index];
    Sample* sample_indexPlusOne = samples[index+1];
    
    double sample_index_currScore =   sample_index->GetTemperedScore(temperatures[index]); 
    double sample_indexPlusOne_currScore = sample_indexPlusOne->GetTemperedScore(temperatures[index+1]); 
    
    double sample_index_swapScore = sample_index->GetTemperedScore(temperatures[index+1]); 
    double sample_indexPlusOne_swapScore = sample_indexPlusOne->GetTemperedScore(temperatures[index]);
    
    VERBOSE (2, "Sample at index " << index << " [temp=" << temperatures[index] << "] has score: " << sample_index_currScore << endl)
    VERBOSE (2, "Sample at index " << index +1  << " [temp=" << temperatures[index+1] << "] has score: " << sample_indexPlusOne_currScore << endl)
    
    VERBOSE (2, "Sample at index " << index << " [temp=" << temperatures[index+1] << "] will have score: " << sample_index_swapScore << endl)
    VERBOSE (2, "Sample at index " << index +1  << " [temp=" << temperatures[index] << "] will have score: " << sample_indexPlusOne_swapScore << endl)
    
    double ratio = (sample_index_swapScore +  sample_indexPlusOne_swapScore) - (sample_index_currScore + sample_indexPlusOne_currScore) ;
    VERBOSE (2, "Accpetrance ratio: " << exp(ratio) << endl);
                   
    bool exchange = false;
    if (ratio >= 0) {
      exchange = true;
      VERBOSE(2, "Exchaging" << endl)
    }
    else {
      random =  log(RandomNumberGenerator::instance().next());
      if (ratio >= random) {//accept with prob random 
        exchange = true;
        VERBOSE(2, "Exchaging with prob " << exp(ratio) << endl)
      } 
    }
                          
    if (exchange) {
      Sample* temp = sample_index;
      samples[index] = sample_indexPlusOne;
      samples[index+1] = temp;
    }
    
    if (index  >= 2) {
      vector <Sample*> leftSamples;
      vector <float> leftTemps;
      for (size_t i = 0; i < index; ++i) {
        leftSamples.push_back(samples[i]);
        leftTemps.push_back(temperatures[i]);
      }
      exchangeSamples(leftSamples, leftTemps); //exchange
      for (size_t i = 0; i < index; ++i) { //now merge
        samples[i] = leftSamples[i];
      }
    }
    if (numChains - index >= 4) {
      vector <Sample*> rightSamples;
      vector <float> rightTemps;
      for (size_t i = index+2; i < numChains; ++i) {
        rightSamples.push_back(samples[i]);
        rightTemps.push_back(temperatures[i]);
      }
      exchangeSamples(rightSamples, rightTemps); //exchange
      for (size_t i = 0; i < rightSamples.size(); ++i) { //now merge
        samples[index+2+i] = rightSamples[i];
      }
    }
  }
  
  void Sampler::collectSample(Sample& sample) {
    //currently feature_values contains the approx scores. The true score = imp score + approx score
    //importance weight
    ScoreComponentCollection importanceScores;
    for (Josiah::feature_vector::const_iterator j = sample.extra_features().begin(); j != sample.extra_features().end(); ++j) {
      (*j)->assignImportanceScore(importanceScores);
    }
    const vector<float> & weights = StaticData::Instance().GetAllWeights();
    //copy(weights.begin(), weights.end(), ostream_iterator<float>(cerr," "));
    //cerr << endl;
    float importanceWeight  = importanceScores.InnerProduct(weights);
    VERBOSE(2, "Importance scores: " << importanceScores << endl);
    VERBOSE(2, "Total importance weight: " << importanceWeight << endl);
    
    sample.UpdateFeatureValues(importanceScores);
    for (size_t j = 0; j < m_collectors.size(); ++j) {
      m_collectors[j]->addSample(sample,importanceWeight);
    }
    //set sample back to be the approx score
    ScoreComponentCollection minusDeltaFV;
    minusDeltaFV.MinusEquals(importanceScores);
    sample.UpdateFeatureValues(minusDeltaFV);
  }
}

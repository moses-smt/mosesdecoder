#include "Sampler.h"
#include "StaticData.h"
#include "GibbsOperator.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "Gibbler.h"
#include "SampleAcceptor.h"
#include "SampleCollector.h"
#include "StopStrategy.h"

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
  
  void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const Josiah::feature_vector& extra_fv, SampleAcceptor* acceptor, bool collectAllSamples, bool defaultCtrIncrementer, bool raoBlackwell) {
    if (m_runRandom)
      RunRandom(starting, options, source, extra_fv, acceptor, collectAllSamples, defaultCtrIncrementer, raoBlackwell, m_lag);
    else
      RunSequential(starting, options, source, extra_fv, acceptor, collectAllSamples, defaultCtrIncrementer, raoBlackwell);
  }
  
  void Sampler::RunSequential(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const Josiah::feature_vector& extra_fv, SampleAcceptor* acceptor, bool collectAllSamples, bool defaultCtrIncrementer, bool raoBlackwell) {
    Sample sample(starting,source,extra_fv,raoBlackwell);
    ResetGainStats();
    bool f = false;
    
    for (size_t j = 0; j < m_operators.size(); ++j) {
      m_operators[j]->addSampleAcceptor(acceptor); 
    }
    
    for (size_t k = 0; k < m_reheatings; ++k) {
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
          VERBOSE(2,"Sampling with operator " << m_operators[j]->name() << endl);
          m_operators[j]->resetIterator(); 
          if (m_as)
            m_operators[j]->SetAnnealingTemperature(m_as->GetTemperatureAtTime(i));
          
          while (m_operators[j]->keepGoing() && i < m_burninIts) {
            m_operators[j]->scan(sample,*options);  
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
      
      
      size_t i = 0;
      size_t allSamples = 0;
      bool keepGoing = true;
      while(keepGoing) {
        if ((i+1) % 5 == 0) { VERBOSE(1,'.'); f=true; }
        if ((i+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
        VERBOSE(2,"Gibbs sampling iteration: " << i << endl);
        for (size_t j = 0; j < m_operators.size() && keepGoing; ++j) {
          m_operators[j]->resetIterator(); 
          VERBOSE(2,"Sampling with operator " << m_operators[j]->name() << endl);
          while (m_operators[j]->keepGoing() && keepGoing) {
            m_operators[j]->scan(sample,*options); 
            allSamples++;
          }
          if (collectAllSamples) {
              collectSample(sample);
              if (!defaultCtrIncrementer)
                ++i;
              if (m_stopper->ShouldStop(i)) {
                keepGoing = false;
                break;
              }
          }
        }
        if (!collectAllSamples) {
          collectSample(sample);
        }  
        if (defaultCtrIncrementer) {
          ++i;
        }
        if (m_stopper->ShouldStop(i)) {
          keepGoing = false;
          break;
        }
        if (collectAllSamples && m_collectors[0]->N() > SAMPLEMAX) {
          keepGoing = false;
          break;
        }
          
      }
      cerr << "Sampled " << allSamples << ", collected " << i << endl;
      if (f) VERBOSE(1,endl);
    }

  }
  
  
  GibbsOperator* Sampler::SampleNextOperator(const std::vector<GibbsOperator*>& operators) {
    double random =  RandomNumberGenerator::instance().next();
    
    size_t position = 1;
    double sum = operators[0]->GetScanProb();
    for (; position < operators.size() && sum < random; ++position) {
      sum += operators[position]->GetScanProb();
    }
    return operators[position-1];
  }
  
  void Sampler::RunRandom(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const Josiah::feature_vector& extra_fv, SampleAcceptor* acceptor, bool collectAllSamples, bool defaultCtrIncrementer, bool raoBlackwell, size_t Lag) {
    Sample sample(starting,source,extra_fv,raoBlackwell);
    ResetGainStats();
    bool f = false;
    
    for (size_t j = 0; j < m_operators.size(); ++j) {
      m_operators[j]->addSampleAcceptor(acceptor); 
    }
    
    for (size_t k = 0; k < m_reheatings; ++k) {
      if (m_burninIts) {
        for (size_t j = 0; j < m_operators.size(); ++j) {
          m_operators[j]->disableGainFunction(); // do not compute gains during burn-in
        } 
      }
      
      //do some burn-in
      size_t allSamples = 0;
      for (size_t its = 0; its < m_burninIts; ++allSamples) {
        if ((its+1) % 5 == 0) { VERBOSE(1,'.'); f=true;}
        if ((its+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
        VERBOSE(2,"Gibbs burnin iteration: " << its << endl);
       
        GibbsOperator* currOperator = SampleNextOperator(m_operators);
        if (m_as)
          currOperator->SetAnnealingTemperature(m_as->GetTemperatureAtTime(its));
        
        currOperator->scan(sample,*options);
        if (allSamples % Lag == 0) //increment now
          ++its;
      }                  
      
      //Done with burn-in, let's get ready for proper sampling
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
      
      //Sample now
      size_t i = 0;
      bool keepGoing = true;
      allSamples = 0;
      while(keepGoing) {
        if ((i+1) % 5 == 0) { VERBOSE(1,'.'); f=true; }
        if ((i+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
        VERBOSE(2,"Gibbs sampling iteration: " << i << endl);
        
        GibbsOperator* currOperator = SampleNextOperator(m_operators);
        currOperator->scan(sample,*options);
        
        if (allSamples % Lag == 0) {//collect and increment now
          collectSample(sample);
          ++i;  
        }
        
        if (m_stopper->ShouldStop(i)) {
          keepGoing = false;
          break;
        }
        if (collectAllSamples && m_collectors[0]->N() > SAMPLEMAX) {
          keepGoing = false;
          break;
        }
        ++allSamples;
      } 
      
      cerr << "Sampled " << allSamples << ", collected " << i << endl;
      if (f) VERBOSE(1,endl);
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
    sample.ResetConditionalFeatureValues();
  }
}

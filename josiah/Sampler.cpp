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
  }
  
  void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const Josiah::feature_vector& extra_fv, SampleAcceptor* acceptor) {
    Sample sample(starting,source,extra_fv);
    
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
          if (m_as)
            m_operators[j]->SetAnnealingTemperature(m_as->GetTemperatureAtTime(i));
          m_operators[j]->doIteration(sample,*options);
        }
      }
      if (f) VERBOSE(1,endl);
      if (m_as) {
        for (size_t j = 0; j < m_operators.size(); ++j)
          m_operators[j]->Quench();
      }
      
      for (size_t j = 0; j < m_operators.size(); ++j) {
        m_operators[j]->enableGainFunction();
        m_operators[j]->addSampleAcceptor(acceptor);
      }
      
      
      size_t i = 0;
      while(true) {
        if ((i+1) % 5 == 0) { VERBOSE(1,'.'); f=true; }
        if ((i+1) % 400 == 0) { VERBOSE(1,endl); f=false;}
        VERBOSE(2,"Gibbs sampling iteration: " << i << endl);
        for (size_t j = 0; j < m_operators.size(); ++j) {
          VERBOSE(2,"Sampling with operator " << m_operators[j]->name() << endl);
          m_operators[j]->doIteration(sample,*options);
        }
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
        ++i;
        if (m_stopper->ShouldStop(i)) {
          break;
        }
      }
      if (f) VERBOSE(1,endl);
      
    }
  }
  
  
}
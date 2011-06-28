

#include "Sampler.h"
#include "GibbsOperator.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "Gibbler.h"
#include "SampleCollector.h"

using namespace std;


namespace Josiah {

  

  void Sampler::AddOperator(GibbsOperator* o) {
    m_operators.push_back(o); 
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
  
  void Sampler::Run(const vector<TranslationHypothesis>& translations, const FeatureVector& features,  bool raoBlackwell) {
    SampleVector samples;
    for (size_t i = 0; i < translations.size(); ++i) {
      samples.push_back(SampleHandle(new Sample(translations[i].getHypothesis(), 
                          translations[i].getWords(), features, raoBlackwell)));
    }
    m_selector->SetSamples(samples);
    
    map<GibbsOperator*, size_t> samplesPerOperator; // to keep track of number of samples per operator
    
   
    for (size_t k = 0; k < m_reheatings; ++k) {
      if (m_burninIts) {
        m_selector->BeginBurnin();
        //do some burn-in
        size_t allSamples = 0;
        for (size_t its = 0; its < m_burninIts; ++allSamples) {
          VERBOSE(2,"Gibbs burnin iteration: " << its << endl);
          doSample(samples,translations,its);
          
          if (allSamples % m_lag == 0) //increment now
            ++its;
        }                  
        m_selector->EndBurnin();
      }
      
      //Sample now
      size_t samplesCollected = 0;
      size_t allSamples = 0;
      while(samplesCollected < m_iterations) {
        VERBOSE(2,"Gibbs sampling iteration: " << allSamples << "Collected: " << samplesCollected << endl);
        
        GibbsOperator* currOperator = doSample(samples,translations,samplesCollected);
        
        if (currOperator) {
          ++samplesPerOperator[currOperator];
          ++allSamples;
          if (allSamples % m_lag == 0) {//collect and increment now
            collectSample(*samples[0]);
            ++samplesCollected;  
          }
        }
      } 
      
      VERBOSE(1,"Sampled " << allSamples << ", collected " << samplesCollected << endl);
      IFVERBOSE(1) {
        for (map<GibbsOperator*, size_t>::const_iterator it = samplesPerOperator.begin(); it != samplesPerOperator.end(); ++it) {
          cerr << "Sampled operator " << (it->first)->name() << ": " << it->second << " times." << endl;
        }  
      }
    }
  }
  
  void Sampler::collectSample(Sample& sample) {
    for (size_t j = 0; j < m_collectors.size(); ++j) {
      m_collectors[j]->addSample(sample);
    }
    
    sample.ResetConditionalFeatureValues(); //for Rao-Blackellisation
  }
  
  GibbsOperator* Sampler::doSample(const SampleVector& samples,
                                   const vector<TranslationHypothesis>& translations, 
                                   size_t iteration) {
    //choose an operator, and sample
    GibbsOperator* currOperator = SampleNextOperator(m_operators);
    TDeltaHandle noChangeDelta;
    TDeltaVector deltas;
    size_t sampleIndex = RandomNumberGenerator::instance().getRandomIndexFromZeroToN(samples.size());
    currOperator->propose(*samples[sampleIndex],*(translations[sampleIndex].getToc()),deltas,noChangeDelta);
    VERBOSE(2,"Created " << deltas.size() << " delta(s) with operator " << currOperator->name() << endl);
    if (deltas.size()) {
      TDeltaHandle selectedDelta = m_selector->Select(sampleIndex, deltas,noChangeDelta, iteration);
      if (selectedDelta.get() != noChangeDelta.get()) {
        selectedDelta->apply(*noChangeDelta);
        if (m_checkFeatures) {
          samples[sampleIndex]->CheckFeatureConsistency();
        }
      }
      return currOperator;
    } else {
      return NULL;
    }
    
    //cerr << "Sampled sentence " << sampleIndex << " and updated to "  << endl;
    //for (size_t i = 0 ; i < samples.size(); ++i) {
    /*  const vector<Word>& words = samples[sampleIndex]->GetTargetWords();
      for (size_t j = 0; j < words.size(); ++j) {
        cerr << words[j];
      }
      cerr << endl;
      cerr << "FV " << samples[sampleIndex]->GetFeatureValues() << endl;
      cerr << "Iteration " << iteration << " Score " << inner_product(samples[sampleIndex]->GetFeatureValues(),
            WeightManager::instance().get()) << endl; */
    //}

  }
}

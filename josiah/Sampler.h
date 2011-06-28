#pragma once

#include <vector>

#include <boost/shared_ptr.hpp>

#include "Word.h"

#include "AnnealingSchedule.h"
#include "Decoder.h"
#include "FeatureFunction.h"
#include "Gibbler.h"
#include "Selector.h"




namespace Moses {
  class Hypothesis;
  class TranslationOptionCollection;
  class Word;
}

using namespace Moses;

namespace Josiah {
  
  

class SampleCollector;
class GibbsOperator; 

#define SAMPLEMAX 1000000
  
class Sampler {
private:
  std::vector<SampleCollector*> m_collectors;
  std::vector<GibbsOperator*> m_operators;
  DeltaSelector* m_selector;
  size_t m_iterations;
  size_t m_burninIts;
  size_t m_reheatings;
  const AnnealingSchedule* m_as;
  size_t m_lag;
  bool m_checkFeatures;
  
  void collectSample(Sample& sample);
  GibbsOperator* SampleNextOperator(const std::vector<GibbsOperator*>& );
  GibbsOperator* doSample(const SampleVector& samples,
                          const std::vector<TranslationHypothesis>& translations, 
                          size_t iteration);
  
public:
  Sampler(): m_selector(NULL), m_iterations(10), m_reheatings(1), m_as(NULL),
  m_lag(0), m_checkFeatures(false) {}
  void Run(const std::vector<TranslationHypothesis>& translations, 
           const FeatureVector& features,
           bool raoBlackwell = false) ;
  void AddOperator(GibbsOperator* o);
  void AddCollector(SampleCollector* c) {m_collectors.push_back(c);}
  void SetSelector(DeltaSelector* selector) {m_selector = selector;}
  void SetIterations(size_t iterations) {m_iterations = iterations;}
  void SetReheatings(size_t r) {m_reheatings = r;}
  void SetLag(size_t l) {m_lag = l;}
  void SetBurnIn(size_t burnin_its) {m_burninIts = burnin_its;}
  void SetCheckFeatures(bool checkFeatures) {m_checkFeatures = checkFeatures;}
  
};

}

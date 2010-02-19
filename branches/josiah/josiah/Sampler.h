#pragma once
#include "FeatureFunction.h"
#include "SufficientStats.h"
#include "Word.h"
#include "AnnealingSchedule.h"
#include <vector>

namespace Moses {
  class Hypothesis;
  class TranslationOptionCollection;
  class Word;
}

using namespace Moses;

namespace Josiah {

class SampleCollector;
class AnnealingSchedule;
class GibbsOperator;
class OnlineLearner;
class StopStrategy;
class SampleAcceptor;  
class TranslationDelta;

#define SAMPLEMAX 1000000
  
class Sampler {
private:
  std::vector<SampleCollector*> m_collectors;
  std::vector<GibbsOperator*> m_operators;
  size_t m_iterations;
  size_t m_burninIts;
  size_t m_reheatings;
  const AnnealingSchedule* m_as;
  float m_quenchTemp;
  StopStrategy* m_stopper;
  OnlineLearner* m_onlineLearner;
  BleuSufficientStats m_suffStats;
  BleuSufficientStats m_averageSuffStats;
  size_t m_numSuffStats;
  ScoreComponentCollection m_GainOptimalSol;
  float m_optimalGain;
  bool m_runRandom;
  size_t m_lag;
public:
  Sampler(): m_iterations(10), m_reheatings(1), m_as(NULL), m_quenchTemp(1.0){}
  void Run(Hypothesis* starting, const TranslationOptionCollection* options, 
           const std::vector<Word>& source, const feature_vector& extra_fv, SampleAcceptor*, bool collectAll = false, bool defaultCtrIncrementer = true, bool raoBlackwell = false) ;
  void RunSequential(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const feature_vector& extra_fv, SampleAcceptor* acceptor, bool collectAllSamples, bool defaultCtrIncrementer, bool raoBlackwell) ;
  void RunRandom(Hypothesis* starting, const TranslationOptionCollection* options, const std::vector<Word>& source, const feature_vector& extra_fv, SampleAcceptor* acceptor, bool collectAllSamples, bool defaultCtrIncrementer, bool raoBlackwell, size_t);
  void AddOperator(GibbsOperator* o);
  void AddCollector(SampleCollector* c) {m_collectors.push_back(c);}
  void SetAnnealingSchedule(const AnnealingSchedule* as) {m_as = as;}
  void SetQuenchingTemperature(float temp) {std::cerr << "Setting quench temp to " << temp << std::endl; m_quenchTemp = temp;}
  void SetIterations(size_t iterations) {m_iterations = iterations;}
  void SetStopper(StopStrategy* stopper) {m_stopper = stopper;}
  void SetReheatings(size_t r) {m_reheatings = r;}
  void SetRandomScan(bool r) {m_runRandom = r;}
  void SetLag(size_t l) {m_lag = l;}
  void SetBurnIn(size_t burnin_its) {m_burninIts = burnin_its;}
  void AddOnlineLearner(OnlineLearner* learner) {m_onlineLearner = learner;}
  OnlineLearner* GetOnlineLearner()  {return m_onlineLearner;}
  void UpdateGainFunctionStats(const BleuSufficientStats& stats);
  BleuSufficientStats* GetSentenceGainFunctionStats();
  void ResetGainStats();
  void SetOptimalGain(float gain) { m_optimalGain = gain;}
  float GetOptimalGain() {return m_optimalGain;}
  void SetOptimalGainSol(TranslationDelta*, TranslationDelta*);
  const ScoreComponentCollection & GetOptimalGainSol() { return m_GainOptimalSol;}
  void collectSample(Sample& sample);
  GibbsOperator* SampleNextOperator(const std::vector<GibbsOperator*>& );
};

}

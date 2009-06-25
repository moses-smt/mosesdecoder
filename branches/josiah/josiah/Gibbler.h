#pragma once

#include <map>
#include <vector>

#include "FeatureFunction.h"
#include "GibbsOperator.h"
#include "ScoreComponentCollection.h"

namespace Moses {
class Hypothesis;
class TranslationOptionCollection;
class TranslationOption;
class Word;
}

using namespace Moses;
namespace Josiah {

class AnnealingSchedule;
class GibbsOperator;
class FeatureFunction;
class Sampler;


  
class Sample {
 private:
  std::vector<Word> m_targetWords;
  const std::vector<Word>& m_sourceWords;

  Hypothesis* target_head;
  Hypothesis* target_tail;

  Hypothesis* source_head;
  Hypothesis* source_tail;

  ScoreComponentCollection feature_values;
  feature_vector _extra_features;

  std::set<Hypothesis*> cachedSampledHyps;
  
  std::map<size_t, Hypothesis*>  sourceIndexedHyps;
  
  void SetSourceIndexedHyps(Hypothesis* h);
  void UpdateFeatureValues(const ScoreComponentCollection& deltaFV);
  void UpdateTargetWordRange(Hypothesis* hyp, int tgtSizeChange);   
  void UpdateHead(Hypothesis* currHyp, Hypothesis* newHyp, Hypothesis *&head);
  void UpdateCoverageVector(Hypothesis& hyp, const TranslationOption& option) ;  
  Hypothesis* CreateHypothesis( Hypothesis& prevTarget, const TranslationOption& option);
  
  void SetTgtNextHypo(Hypothesis*  newHyp, Hypothesis* currNextHypo);
  void SetSrcPrevHypo(Hypothesis*  newHyp, Hypothesis* srcPrevHypo);
  void UpdateTargetWords();
  void DeleteFromCache(Hypothesis *hyp);
  float ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) ;
 public:
  Sample(Hypothesis* target_head, const std::vector<Word>& source, const feature_vector& extra_features);
  ~Sample();
  int GetSourceSize() const { return m_sourceWords.size(); }
  Hypothesis* GetHypAtSourceIndex(size_t ) ;
  const Hypothesis* GetSampleHypothesis() const {
    return target_head;
  }
  
  const Hypothesis* GetTargetTail() const {
    return target_tail;
  }
  
  const ScoreComponentCollection& GetFeatureValues() const {
    return feature_values;
  }
  
  const feature_vector& extra_features() const {
    return _extra_features; 
  }
  
  void FlipNodes(size_t x, size_t y, const ScoreComponentCollection& deltaFV) ;
  void FlipNodes(const TranslationOption& , const TranslationOption&, Hypothesis* , Hypothesis* , const ScoreComponentCollection& deltaFV);
  void ChangeTarget(const TranslationOption& option, const ScoreComponentCollection& deltaFV); 
  void MergeTarget(const TranslationOption& option, const ScoreComponentCollection& deltaFV);
  void SplitTarget(const TranslationOption& leftTgtOption, const TranslationOption& rightTgtOption,  const ScoreComponentCollection& deltaFV);
  /** Words in the current target */
  const std::vector<Word>& GetTargetWords() const { return m_targetWords; }
  const std::vector<Word>& GetSourceWords() const { return m_sourceWords; } 
  
  int GetTargetLength()  { return m_targetWords.size(); }
  friend class Sampler;
};



/**
 * Used by the operators to collect samples, for example to count ngrams, or just to print
 * them out. 
 **/
class SampleCollector {
  public:
    SampleCollector(): m_totalImportanceWeight(0), m_n(0)  {}
    virtual void addSample(Sample& sample, double importanceWeight);
    /** Number of samples */
    size_t N() const {return m_n;}
    virtual ~SampleCollector() {}
    void reset() {
      m_totalImportanceWeight = 0;
      m_n = 0;
      m_importanceWeights.clear();
      m_normalisedImportanceWeights.clear();
    }
  protected:
    /** The actual collection.*/
    virtual void collect(Sample& sample) = 0;
    /** The log of the total importance weight */
    float getTotalImportanceWeight() const {return m_totalImportanceWeight;}
    /** Normalised importance weights  - in probability space*/
    const std::vector<double>& getImportanceWeights() const {return m_normalisedImportanceWeights;}
    
    
  private:
    double m_totalImportanceWeight; //normalisation factor
    std::vector<double> m_importanceWeights; //unnormalised weights, in log space
    std::vector<double> m_normalisedImportanceWeights; //normalised, in prob space
    size_t m_n;
};

class PrintSampleCollector  : public virtual SampleCollector {
  public:
    virtual void collect(Sample& sample);
    virtual ~PrintSampleCollector() {}
};



/**
 * Used to specify when the sampler should stop.
 **/
class StopStrategy {
  public:
    virtual bool ShouldStop(size_t iterations) = 0;
    virtual ~StopStrategy() {}
};

/**
 * Simplest sampler stop strategy, just uses the number of iterations.
 **/
class CountStopStrategy : public virtual StopStrategy {
  public:
    CountStopStrategy(size_t max): m_max(max) {}
    virtual bool ShouldStop(size_t iterations) {return iterations >= m_max;}
    virtual ~CountStopStrategy() {}
  
  private:
    size_t m_max;
};




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
 public:
  Sampler(): m_iterations(10), m_reheatings(1), m_as(NULL), m_quenchTemp(1.0) {}
  void Run(Hypothesis* starting, const TranslationOptionCollection* options, 
    const std::vector<Word>& source, const feature_vector& extra_fv) ;
  void RunCollectAll(Hypothesis* starting, const TranslationOptionCollection* options, 
           const std::vector<Word>& source, const feature_vector& extra_fv) ;
  void AddOperator(GibbsOperator* o) {m_operators.push_back(o);}
  void AddCollector(SampleCollector* c) {m_collectors.push_back(c);}
  void SetAnnealingSchedule(const AnnealingSchedule* as) {m_as = as;}
  void SetQuenchingTemperature(float temp) {cerr << "Setting quench temp to " << temp << endl; m_quenchTemp = temp;}
  void SetIterations(size_t iterations) {m_iterations = iterations;}
  void SetStopper(StopStrategy* stopper) {m_stopper = stopper;}
  void SetReheatings(size_t r) {m_reheatings = r;}
  void SetBurnIn(size_t burnin_its) {m_burninIts = burnin_its;}
};



}





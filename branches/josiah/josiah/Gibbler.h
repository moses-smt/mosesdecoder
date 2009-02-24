#pragma once

#include <map>
#include <vector>

#include "FeatureFunction.h"
#include "GibbsOperator.h"
#include "ScoreComponentCollection.h"

namespace Moses {

class AnnealingSchedule;
class GibbsOperator;
class Hypothesis;
class TranslationOptionCollection;
class TranslationOption;
class Word;
class FeatureFunction;

  
class Sample {
 private:
  std::vector<Word> m_targetWords;
  const std::vector<Word>& m_sourceWords;

  Hypothesis* target_head;
  Hypothesis* target_tail;

  Hypothesis* source_head;
  Hypothesis* source_tail;

  ScoreComponentCollection feature_values;
  const Josiah::feature_vector& _extra_features;
  std::vector<float> _extra_feature_values;

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
  Sample(Hypothesis* target_head, const std::vector<Word>& source, const Josiah::feature_vector& extra_features);
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
  const Josiah::feature_vector& extra_features() const { return _extra_features; }
  const std::vector<float>& extra_feature_values() const { return _extra_feature_values; }

  void FlipNodes(size_t x, size_t y, const ScoreComponentCollection& deltaFV) ;
  void FlipNodes(const TranslationOption& , const TranslationOption&, Hypothesis* , Hypothesis* , const ScoreComponentCollection& deltaFV);
  void ChangeTarget(const TranslationOption& option, const ScoreComponentCollection& deltaFV); 
  void MergeTarget(const TranslationOption& option, const ScoreComponentCollection& deltaFV);
  void SplitTarget(const TranslationOption& leftTgtOption, const TranslationOption& rightTgtOption,  const ScoreComponentCollection& deltaFV);
  /** Words in the current target */
  const std::vector<Word>& GetTargetWords() const { return m_targetWords; }
  const std::vector<Word>& GetSourceWords() const { return m_sourceWords; } 
  /** Extra feature functions - not including moses ones */
};

/**
 * Used by the operators to collect samples, for example to count ngrams, or just to print
 * them out. 
 **/
class SampleCollector {
  public:
    virtual void collect(Sample& sample) = 0;
    virtual ~SampleCollector() {}
};

class PrintSampleCollector  : public virtual SampleCollector {
  public:
    virtual void collect(Sample& sample);
    virtual ~PrintSampleCollector() {}
};

class Sampler {
 private:
   std::vector<SampleCollector*> m_collectors;
   std::vector<GibbsOperator*> m_operators;
   size_t m_iterations;
   size_t m_burninIts;
   const AnnealingSchedule* m_as;
 public:
  Sampler(): m_iterations(10), m_as(NULL) {}
  void Run(Hypothesis* starting, const TranslationOptionCollection* options, 
    const std::vector<Word>& source, const Josiah::feature_vector& extra_fv) ;
  void AddOperator(GibbsOperator* o) {m_operators.push_back(o);}
  void AddCollector(SampleCollector* c) {m_collectors.push_back(c);}
  void SetAnnealingSchedule(const AnnealingSchedule* as) {m_as = as;}
  void SetIterations(size_t iterations) {m_iterations = iterations;}
  void SetBurnIn(size_t burnin_its) {m_burninIts = burnin_its;}
};



}




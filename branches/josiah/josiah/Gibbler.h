#pragma once

#include <map>
#include <vector>
#include "FeatureFunction.h"

namespace Moses {
class Hypothesis;
class TranslationOptionCollection;
class TranslationOption;
class Word;
class ScoreComponentCollection;
}

using namespace Moses;
namespace Josiah {

class AnnealingSchedule;
class GibbsOperator;
class Sampler;
class OnlineLearner;
class SampleAcceptor;
  
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
  Sample(const Sample&);
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
  
  double GetTemperedScore(float);
  
  friend class Sampler;
  friend class GibbsOperator;
};

}





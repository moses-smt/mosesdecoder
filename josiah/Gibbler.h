#pragma once

#include <map>
#include <set>
#include <vector>
#include "FeatureFunction.h"
#include "FeatureVector.h"

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

  FVector feature_values;
  FeatureFunctionVector m_featureFunctions;

  std::set<Hypothesis*> cachedSampledHyps;
  
  std::map<size_t, Hypothesis*>  sourceIndexedHyps;
  
  //Used for conditional estimation (aka Rao-Blackwellisation)
  bool m_doRaoBlackwell;
  FVector m_conditionalFeatureValues;
  size_t m_updates;
  
  void SetSourceIndexedHyps(Hypothesis* h);
  void UpdateFeatureValues(const FVector& deltaFV);
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
  Sample(Hypothesis* target_head, const std::vector<Word>& source, const FeatureVector& features, bool raoBlackwell);
  ~Sample();
  int GetSourceSize() const { return m_sourceWords.size(); }
  Hypothesis* GetHypAtSourceIndex(size_t ) ;
  const Hypothesis* GetSampleHypothesis() const {
    return target_head;
  }
  
  const Hypothesis* GetTargetTail() const {
    return target_tail;
  }
  
  const FVector& GetFeatureValues() const {
    return feature_values;
  }
  
  const FeatureFunctionVector& GetFeatureFunctions() const {
    return m_featureFunctions; 
  }
  
  /** Check that the feature values are correct */
  void CheckFeatureConsistency() const;
  
  void FlipNodes(size_t x, size_t y, const FVector& deltaFV) ;
  void FlipNodes(const TranslationOption& , const TranslationOption&, Hypothesis* , Hypothesis* , const FVector& deltaFV);
  void ChangeTarget(const TranslationOption& option, const FVector& deltaFV); 
  void MergeTarget(const TranslationOption& option, const FVector& deltaFV);
  void SplitTarget(const TranslationOption& leftTgtOption, const TranslationOption& rightTgtOption,  const FVector& deltaFV);
  /** Words in the current target */
  const std::vector<Word>& GetTargetWords() const { return m_targetWords; }
  const std::vector<Word>& GetSourceWords() const { return m_sourceWords; } 
  
  int GetTargetLength()  { return m_targetWords.size(); }
  
  //Used for conditional estimation (aka Rao-Blackwellisation)
  bool DoRaoBlackwell() const;
  void AddConditionalFeatureValues(const FVector& fv);
  void ResetConditionalFeatureValues();
  const FVector GetConditionalFeatureValues() const;
  
  friend class Sampler;
  friend class GibbsOperator;
};

typedef boost::shared_ptr<Sample> SampleHandle;
typedef std::vector<SampleHandle> SampleVector;

}





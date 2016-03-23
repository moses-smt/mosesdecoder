#pragma once

#include <string>
#include <map>
#include <limits>
#include <vector>

#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>

#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/PP/CountsPhraseProperty.h"
#include "moses/TranslationOptionList.h"
#include "moses/TranslationOption.h"
#include "moses/Util.h"
#include "moses/TypeDef.h"
#include "moses/StaticData.h"
#include "moses/Phrase.h"
#include "moses/AlignmentInfo.h"
#include "moses/Word.h"
#include "moses/FactorCollection.h"

#include "Normalizer.h"
#include "Classifier.h"
#include "VWFeatureBase.h"
#include "TabbedSentence.h"
#include "ThreadLocalByFeatureStorage.h"
#include "TrainingLoss.h"
#include "VWTargetSentence.h"

namespace Moses
{

// dummy class label; VW does not use the actual label, other classifiers might
const std::string VW_DUMMY_LABEL = "1111";

// thread-specific classifier instance
typedef ThreadLocalByFeatureStorage<Discriminative::Classifier, Discriminative::ClassifierFactory &> TLSClassifier;

// current target sentence, used in VW training (vwtrainer), not in decoding (prediction time)
typedef ThreadLocalByFeatureStorage<VWTargetSentence> TLSTargetSentence;

typedef boost::unordered_map<size_t, Discriminative::FeatureVector> FeatureVectorMap;
typedef ThreadLocalByFeatureStorage<FeatureVectorMap> TLSFeatureVectorMap;

typedef boost::unordered_map<size_t, float> FloatHashMap;
typedef ThreadLocalByFeatureStorage<FloatHashMap> TLSFloatHashMap;
typedef ThreadLocalByFeatureStorage<boost::unordered_map<size_t, FloatHashMap> > TLSStateExtensions;

class VW : public StatefulFeatureFunction, public TLSTargetSentence
{
public:
  VW(const std::string &line);

  virtual ~VW();

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
  }

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  virtual FFState* EvaluateWhenApplied(
    const Hypothesis& curHypo,
    const FFState* prevState,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied(
    const ChartHypothesis&,
    int,
    ScoreComponentCollection* accumulator) const { 
    throw new std::logic_error("hiearchical/syntax not supported"); 
  }

  const FFState* EmptyHypothesisState(const InputType &input) const; 

  virtual void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const;

  void SetParameter(const std::string& key, const std::string& value);

  virtual void InitializeForInput(ttasksptr const& ttask);

private:
  inline std::string MakeTargetLabel(const TargetPhrase &targetPhrase) const {
    return VW_DUMMY_LABEL;
  }

  inline size_t MakeCacheKey(const FFState *prevState, size_t spanStart, size_t spanEnd) const {
    size_t key = 0;
    boost::hash_combine(key, prevState);
    boost::hash_combine(key, spanStart);
    boost::hash_combine(key, spanEnd);
    return key;
  }

  std::pair<bool, int> IsCorrectTranslationOption(const TranslationOption &topt) const;

  std::vector<bool> LeaveOneOut(const TranslationOptionList &topts, const std::vector<bool> &correct) const;

  bool m_train; // false means predict
  std::string m_modelPath;
  std::string m_vwOptions;

  Word m_sentenceStartWord;

  // calculator of training loss
  TrainingLoss *m_trainingLoss = NULL;

  // optionally contains feature name of a phrase table where we recompute scores with leaving one out
  std::string m_leaveOneOut;

  Discriminative::Normalizer *m_normalizer = NULL;
  TLSClassifier *m_tlsClassifier;

  TLSFloatHashMap *m_tlsFutureScores;
  TLSStateExtensions *m_tlsComputedStateExtensions;
  TLSFeatureVectorMap *m_tlsTranslationOptionFeatures, *m_tlsTargetContextFeatures;
};

}

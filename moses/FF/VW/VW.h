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

/*
 * VW classifier feature. See vw/README.md for further information.
 *
 * TODO: say which paper to cite.
 */

namespace Moses
{

// dummy class label; VW does not use the actual label, other classifiers might
const std::string VW_DUMMY_LABEL = "1111";

// thread-specific classifier instance
typedef ThreadLocalByFeatureStorage<Discriminative::Classifier, Discriminative::ClassifierFactory &> TLSClassifier;

// current target sentence, used in VW training (vwtrainer), not in decoding (prediction time)
typedef ThreadLocalByFeatureStorage<VWTargetSentence> TLSTargetSentence;

// hash table of feature vectors
typedef boost::unordered_map<size_t, Discriminative::FeatureVector> FeatureVectorMap;

// thread-specific feature vector hash
typedef ThreadLocalByFeatureStorage<FeatureVectorMap> TLSFeatureVectorMap;

// hash table of partial scores
typedef boost::unordered_map<size_t, float> FloatHashMap;

// thread-specific score hash table, used for caching
typedef ThreadLocalByFeatureStorage<FloatHashMap> TLSFloatHashMap;

// thread-specific hash tablei for caching full classifier outputs
typedef ThreadLocalByFeatureStorage<boost::unordered_map<size_t, FloatHashMap> > TLSStateExtensions;

/*
 * VW feature function. A discriminative classifier with source and target context features.
 */
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

  // This behavior of this method depends on whether it's called during VW
  // training (feature extraction) by vwtrainer or during decoding (prediction
  // time) by Moses.
  //
  // When predicting, it evaluates all translation options with the VW model;
  // if no target-context features are defined, this is the final score and it
  // is added directly to the TranslationOption score. If there are target
  // context features, the score is a partial score and it is only stored in
  // cache; the final score is computed based on target context in
  // EvaluateWhenApplied().
  //
  // This method is also used in training by vwtrainer in which case features
  // are written to a file, no classifier predictions take place. Target-side
  // context is constant at training time (we know the true target sentence),
  // so target-context features are extracted here as well.
  virtual void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const;

  // Evaluate VW during decoding. This is only used at prediction time (not in training).
  // When no target-context features are defined, VW predictions were already fully calculated
  // in EvaluateTranslationOptionListWithSourceContext() and the scores were added to the model.
  // If there are target-context features, we compute the context-dependent part of the
  // classifier score and combine it with the source-context only partial score which was computed
  // in EvaluateTranslationOptionListWithSourceContext(). Various caches are used to make this
  // method more efficient.
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

  // Initial VW state; contains unaligned BOS symbols.
  const FFState* EmptyHypothesisState(const InputType &input) const;

  void SetParameter(const std::string& key, const std::string& value);

  // At prediction time, this clears our caches. At training time, we load the next sentence, its
  // translation and word alignment.
  virtual void InitializeForInput(ttasksptr const& ttask);

private:
  inline std::string MakeTargetLabel(const TargetPhrase &targetPhrase) const {
    return VW_DUMMY_LABEL; // VW does not care about class labels in our setting (--csoaa_ldf mc).
  }

  inline size_t MakeCacheKey(const FFState *prevState, size_t spanStart, size_t spanEnd) const {
    size_t key = 0;
    boost::hash_combine(key, prevState);
    boost::hash_combine(key, spanStart);
    boost::hash_combine(key, spanEnd);
    return key;
  }

  // used in decoding to transform the global word alignment information into
  // context-phrase internal alignment information (i.e., with target indices correspoding
  // to positions in contextPhrase)
  const AlignmentInfo *TransformAlignmentInfo(const Hypothesis &curHypo, size_t contextSize) const;

  // used during training to extract relevant alignment points from the full sentence alignment
  // and shift them by target context size
  AlignmentInfo TransformAlignmentInfo(const AlignmentInfo &alignInfo, size_t contextSize, int currentStart) const;

  // At training time, determine whether a translation option is correct for the current target sentence
  // based on word alignment. This is a bit complicated because we need to handle various corner-cases
  // where some word(s) on phrase borders are unaligned.
  std::pair<bool, int> IsCorrectTranslationOption(const TranslationOption &topt) const;

  // At training time, optionally discount occurrences of phrase pairs from the current sentence, helps prevent
  // over-fitting.
  std::vector<bool> LeaveOneOut(const TranslationOptionList &topts, const std::vector<bool> &correct) const;

  bool m_train; // false means predict
  std::string m_modelPath; // path to the VW model file; at training time, this is where extracted features are stored
  std::string m_vwOptions; // options for Vowpal Wabbit

  // BOS token, all factors
  Word m_sentenceStartWord;

  // calculator of training loss
  TrainingLoss *m_trainingLoss = NULL;

  // optionally contains feature name of a phrase table where we recompute scores with leaving one out
  std::string m_leaveOneOut;

  // normalizer, typically this means softmax
  Discriminative::Normalizer *m_normalizer = NULL;

  // thread-specific classifier instance
  TLSClassifier *m_tlsClassifier;

  // caches for partial scores and feature vectors
  TLSFloatHashMap *m_tlsFutureScores;
  TLSStateExtensions *m_tlsComputedStateExtensions;
  TLSFeatureVectorMap *m_tlsTranslationOptionFeatures, *m_tlsTargetContextFeatures;
};

}

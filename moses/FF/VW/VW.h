#pragma once

#include <string>
#include <map>
#include <limits>

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

namespace Moses
{

const std::string VW_DUMMY_LABEL = "1111"; // VW does not use the actual label, other classifiers might

/**
 * Helper class for storing alignment constraints.
 */
class Constraint
{
public:
  Constraint() : m_min(std::numeric_limits<int>::max()), m_max(-1) {}

  Constraint(int min, int max) : m_min(min), m_max(max) {}

  /**
   * We are aligned to point => our min cannot be larger, our max cannot be smaller.
   */
  void Update(int point) {
    if (m_min > point) m_min = point;
    if (m_max < point) m_max = point;
  }

  bool IsSet() const {
    return m_max != -1;
  }

  int GetMin() const {
    return m_min;
  }

  int GetMax() const {
    return m_max;
  }

private:
  int m_min, m_max;
};

/**
 * VW thread-specific data about target sentence.
 */
struct VWTargetSentence {
  VWTargetSentence() : m_sentence(NULL), m_alignment(NULL) {}

  void Clear() {
    if (m_sentence) delete m_sentence;
    if (m_alignment) delete m_alignment;
  }

  ~VWTargetSentence() {
    Clear();
  }

  void SetConstraints(size_t sourceSize) {
    // initialize to unconstrained
    m_sourceConstraints.assign(sourceSize, Constraint());
    m_targetConstraints.assign(m_sentence->GetSize(), Constraint());

    // set constraints according to alignment points
    AlignmentInfo::const_iterator it;
    for (it = m_alignment->begin(); it != m_alignment->end(); it++) {
      int src = it->first;
      int tgt = it->second;

      if (src >= m_sourceConstraints.size() || tgt >= m_targetConstraints.size()) {
        UTIL_THROW2("VW :: alignment point out of bounds: " << src << "-" << tgt);
      }

      m_sourceConstraints[src].Update(tgt);
      m_targetConstraints[tgt].Update(src);
    }
  }

  Phrase *m_sentence;
  AlignmentInfo *m_alignment;
  std::vector<Constraint> m_sourceConstraints, m_targetConstraints;
};

/**
 * VW state, used in decoding (when target context is enabled).
 */
struct VWState : public FFState {
  virtual size_t hash() const {
    return hash_value(m_phrase);
  }

  virtual bool operator==(const FFState& o) const {
    const VWState &other = static_cast<const VWState &>(o);
    return m_phrase == other.m_phrase;
  }

  Phrase m_phrase;
};

// how to print a VW state
std::ostream &operator<<(std::ostream &out, const VWState &state) {
  out << state.m_phrase;
  return out;
}

typedef ThreadLocalByFeatureStorage<Discriminative::Classifier, Discriminative::ClassifierFactory &> TLSClassifier;

typedef ThreadLocalByFeatureStorage<VWTargetSentence> TLSTargetSentence;

typedef boost::unordered_map<size_t, float> FloatHashMap;
typedef ThreadLocalByFeatureStorage<FloatHashMap> TLSFloatHashMap;
typedef ThreadLocalByFeatureStorage<boost::unordered_map<size_t, FloatHashMap> > TLSStateExtensions;

class VW : public StatefulFeatureFunction, public TLSTargetSentence
{
public:
  VW(const std::string &line)
    : StatefulFeatureFunction(1, line)
    , TLSTargetSentence(this)
    , m_train(false)
    , m_sentenceStartWord(Word()) {
    ReadParameters();
    Discriminative::ClassifierFactory *classifierFactory = m_train
        ? new Discriminative::ClassifierFactory(m_modelPath)
        : new Discriminative::ClassifierFactory(m_modelPath, m_vwOptions);

    m_tlsClassifier = new TLSClassifier(this, *classifierFactory);

    m_tlsFutureScores = new TLSFloatHashMap(this);
    m_tlsComputedStateExtensions = new TLSStateExtensions(this);

    if (! m_normalizer) {
      VERBOSE(1, "VW :: No loss function specified, assuming logistic loss.\n");
      m_normalizer = (Discriminative::Normalizer *) new Discriminative::LogisticLossNormalizer();
    }

    if (! m_trainingLoss) {
      VERBOSE(1, "VW :: Using basic 1/0 loss calculation in training.\n");
      m_trainingLoss = (TrainingLoss *) new TrainingLossBasic();
    }

    // create a virtual beginning-of-sentence word with all factors replaced by <S>
    const Factor *bosFactor = FactorCollection::Instance().AddFactor(BOS_);
    for (size_t i = 0; i < MAX_NUM_FACTORS; i++)
      m_sentenceStartWord.SetFactor(i, bosFactor);
  }

  virtual ~VW() {
    delete m_tlsClassifier;
    delete m_normalizer;
  }

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
    ScoreComponentCollection* accumulator) const
  { 
    VERBOSE(2, "VW :: Evaluating translation options\n");

    // which feature functions do we use (on the source and target side)
    const std::vector<VWFeatureBase*>& sourceFeatures =
      VWFeatureBase::GetSourceFeatures(GetScoreProducerDescription());

    const std::vector<VWFeatureBase*>& contextFeatures =
      VWFeatureBase::GetTargetContextFeatures(GetScoreProducerDescription());

    const std::vector<VWFeatureBase*>& targetFeatures =
      VWFeatureBase::GetTargetFeatures(GetScoreProducerDescription());

    size_t maxContextSize = VWFeatureBase::GetMaximumContextSize(GetScoreProducerDescription());

    if (contextFeatures.empty()) {
      // no target context features => we already evaluated everything in
      // EvaluateTranslationOptionListWithSourceContext(). Nothing to do now,
      // no state information to track.
      return new VWState();
    }

    size_t spanStart = curHypo.GetTranslationOption().GetStartPos();
    size_t spanEnd   = curHypo.GetTranslationOption().GetEndPos();
    const Range &sourceRange = curHypo.GetTranslationOption().GetSourceWordsRange();

    // compute our current key
    size_t cacheKey = MakeCacheKey(prevState, spanStart, spanEnd);

    boost::unordered_map<size_t, FloatHashMap> &computedStateExtensions 
      = *m_tlsComputedStateExtensions->GetStored();

    if (computedStateExtensions.find(cacheKey) == computedStateExtensions.end()) {
      // we have not computed this set of translation options yet
      const TranslationOptionList *topts = 
        curHypo.GetManager().getSntTranslationOptions()->GetTranslationOptionList(spanStart, spanEnd);

      const InputType& input = curHypo.GetManager().GetSource();

      Discriminative::Classifier &classifier = *m_tlsClassifier->GetStored();

      // XXX this is a naive implementation, fix this!

      // extract source side features
      for(size_t i = 0; i < sourceFeatures.size(); ++i)
        (*sourceFeatures[i])(input, sourceRange, classifier);

      // extract target context features
      const Phrase &targetContext = static_cast<const VWState *>(prevState)->m_phrase;

      std::vector<std::string> contextExtractedFeatures;
      for(size_t i = 0; i < contextFeatures.size(); ++i)
        (*contextFeatures[i])(targetContext, contextExtractedFeatures);

      for (size_t i = 0; i < contextExtractedFeatures.size(); i++)
        classifier.AddLabelIndependentFeature(contextExtractedFeatures[i]);

      std::vector<float> losses(topts->size());

      for (size_t toptIdx = 0; toptIdx < topts->size(); toptIdx++) {
        const TranslationOption *topt = topts->Get(toptIdx);
        const TargetPhrase &targetPhrase = topt->GetTargetPhrase();

        // extract target-side features for each topt
        for(size_t i = 0; i < targetFeatures.size(); ++i)
          (*targetFeatures[i])(input, targetPhrase, classifier);

        // get classifier score
        losses[toptIdx] = classifier.Predict(MakeTargetLabel(targetPhrase));
      }

      // normalize classifier scores to get a probability distribution
      (*m_normalizer)(losses);

      // fill our cache with the results
      FloatHashMap &toptScores = computedStateExtensions[cacheKey];
      for (size_t toptIdx = 0; toptIdx < topts->size(); toptIdx++) {
        const TranslationOption *topt = topts->Get(toptIdx);
        size_t toptHash = hash_value(*topt);
        toptScores[toptHash] = FloorScore(TransformScore(losses[toptIdx]));
      }

      VERBOSE(3, "VW :: cache miss\n");
    } else {
      VERBOSE(3, "VW :: cache hit\n");
    }

    // now our cache is guaranteed to contain the required score, simply look it up
    std::vector<float> newScores(m_numScoreComponents);
    size_t toptHash = hash_value(curHypo.GetTranslationOption());
    newScores[0] = computedStateExtensions[cacheKey][toptHash];
    VERBOSE(3, "VW :: adding score: " << newScores[0] << "\n");
    accumulator->PlusEquals(this, newScores);


    /*
     * Phrase context = makeContextPhrase(hypo);
     * vector<string> extractedFeatures;
     * for (f : contextfeatures) {
     *   f(context, extractedFeatures);
     * }
     *
     * state = makeState(phrase);
     *
     * contextFeaturesCache[state] = extractedFeatures;
     * //TODO forget caches after each input sentence!
     *
     * spanStart = cur_hypo.GetTranslationOption().GetStartPos();
     * spanEnd   = cur_hypo.GetTranslationOption().GetEndPos();
     *
     * topts = cur_hypo.GetManager().GetSntTranslationOptions().GetTranslationOptionList(spanStart, spanEnd);
     *
     * for (topt : topts) {
     *   vector<int> &tgtFeatures = getTargetFeatures(topt.phrase.hash());
     *   get an ezexample, add hashes of features for target context, add namespace+features for current topts, get dot product
     *   normalize();
     * }
     *
     * cacheKey = makeKey(state, spanStart, spanEnd);
     * targetScoresCache[cacheKey] = vwscores
     */

    return UpdateState(prevState, curHypo);
  }
  

  virtual FFState* EvaluateWhenApplied(
    const ChartHypothesis&,
    int,
    ScoreComponentCollection* accumulator) const
  { 
    throw new std::logic_error("hiearchical/syntax not supported"); 
  }

  const FFState* EmptyHypothesisState(const InputType &input) const {
    size_t maxContextSize = VWFeatureBase::GetMaximumContextSize(GetScoreProducerDescription());
    VWState *initial = new VWState();
    for (size_t i = 0; i < maxContextSize; i++)
      initial->m_phrase.AddWord(m_sentenceStartWord);
      
    return initial;
  }

  virtual void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
    Discriminative::Classifier &classifier = *m_tlsClassifier->GetStored();

    if (translationOptionList.size() == 0)
      return; // nothing to do

    VERBOSE(2, "VW :: Evaluating translation options\n");

    // which feature functions do we use (on the source and target side)
    const std::vector<VWFeatureBase*>& sourceFeatures =
      VWFeatureBase::GetSourceFeatures(GetScoreProducerDescription());

    const std::vector<VWFeatureBase*>& contextFeatures =
      VWFeatureBase::GetTargetContextFeatures(GetScoreProducerDescription());

    const std::vector<VWFeatureBase*>& targetFeatures =
      VWFeatureBase::GetTargetFeatures(GetScoreProducerDescription());

    size_t maxContextSize = VWFeatureBase::GetMaximumContextSize(GetScoreProducerDescription());

    // only use stateful score computation when needed
    bool haveTargetContextFeatures = ! contextFeatures.empty();

    const Range &sourceRange = translationOptionList.Get(0)->GetSourceWordsRange();

    if (m_train) {
      //
      // extract features for training the classifier (only call this when using vwtrainer, not in Moses!)
      //

      // find which topts are correct
      std::vector<bool> correct(translationOptionList.size());
      std::vector<int> startsAt(translationOptionList.size());
      std::set<int> uncoveredStartingPositions;

      for (size_t i = 0; i < translationOptionList.size(); i++) {
        std::pair<bool, int> isCorrect = IsCorrectTranslationOption(* translationOptionList.Get(i));
        correct[i] = isCorrect.first;
        startsAt[i] = isCorrect.second;
        if (isCorrect.first) {
          uncoveredStartingPositions.insert(isCorrect.second);
        }
      }

      // optionally update translation options using leave-one-out
      std::vector<bool> keep = (m_leaveOneOut.size() > 0)
                               ? LeaveOneOut(translationOptionList, correct)
                               : std::vector<bool>(translationOptionList.size(), true);

      while (! uncoveredStartingPositions.empty()) {
        int currentStart = *uncoveredStartingPositions.begin();
        uncoveredStartingPositions.erase(uncoveredStartingPositions.begin());

        // check whether we (still) have some correct translation
        int firstCorrect = -1;
        for (size_t i = 0; i < translationOptionList.size(); i++) {
          if (keep[i] && correct[i] && startsAt[i] == currentStart) {
            firstCorrect = i;
            break;
          }
        }

        // do not train if there are no positive examples
        if (firstCorrect == -1) {
          VERBOSE(2, "VW :: skipping topt collection, no correct translation for span at current tgt start position\n");
          continue;
        }

        // the first correct topt can be used by some loss functions
        const TargetPhrase &correctPhrase = translationOptionList.Get(firstCorrect)->GetTargetPhrase();

        // extract source side features
        for(size_t i = 0; i < sourceFeatures.size(); ++i)
          (*sourceFeatures[i])(input, sourceRange, classifier);

        // build target-side context
        Phrase targetContext;
        for (size_t i = 0; i < maxContextSize; i++)
          targetContext.AddWord(m_sentenceStartWord);

        const Phrase *targetSent = GetStored()->m_sentence;
        if (currentStart > 0)
          targetContext.Append(targetSent->GetSubString(Range(0, currentStart - 1)));

        // extract target-context features
        std::vector<std::string> contextExtractedFeatures;
        for(size_t i = 0; i < contextFeatures.size(); ++i)
          (*contextFeatures[i])(targetContext, contextExtractedFeatures);

        for (size_t i = 0; i < contextExtractedFeatures.size(); i++)
          classifier.AddLabelIndependentFeature(contextExtractedFeatures[i]);

        // go over topts, extract target side features and train the classifier
        for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {

          // this topt was discarded by leaving one out
          if (! keep[toptIdx])
            continue;

          // extract target-side features for each topt
          const TargetPhrase &targetPhrase = translationOptionList.Get(toptIdx)->GetTargetPhrase();
          for(size_t i = 0; i < targetFeatures.size(); ++i)
            (*targetFeatures[i])(input, targetPhrase, classifier);

          bool isCorrect = correct[toptIdx] && startsAt[toptIdx] == currentStart;
          float loss = (*m_trainingLoss)(targetPhrase, correctPhrase, isCorrect);

          // train classifier on current example
          classifier.Train(MakeTargetLabel(targetPhrase), loss);
        }
      }
    } else {
      //
      // predict using a trained classifier, use this in decoding (=at test time)
      //

      std::vector<float> losses(translationOptionList.size());

      // extract source side features
      for(size_t i = 0; i < sourceFeatures.size(); ++i)
        (*sourceFeatures[i])(input, sourceRange, classifier);

      for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {
        const TranslationOption *topt = translationOptionList.Get(toptIdx);
        const TargetPhrase &targetPhrase = topt->GetTargetPhrase();

        // extract target-side features for each topt
        for(size_t i = 0; i < targetFeatures.size(); ++i)
          (*targetFeatures[i])(input, targetPhrase, classifier);

        // get classifier score
        losses[toptIdx] = classifier.Predict(MakeTargetLabel(targetPhrase));
      }

      // normalize classifier scores to get a probability distribution
      std::vector<float> rawLosses = losses;
      (*m_normalizer)(losses);

      // update scores of topts
      for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {
        TranslationOption *topt = *(translationOptionList.begin() + toptIdx);
        if (! haveTargetContextFeatures) {
          // no target context features; evaluate the FF now
          std::vector<float> newScores(m_numScoreComponents);
          newScores[0] = FloorScore(TransformScore(losses[toptIdx]));

          ScoreComponentCollection &scoreBreakDown = topt->GetScoreBreakdown();
          scoreBreakDown.PlusEquals(this, newScores);

          topt->UpdateScore();
        } else {
          // we have target context features => this is just a partial score,
          // do not add it to the score component collection
          size_t toptHash = hash_value(*topt);
          float futureScore = rawLosses[toptIdx];
          m_tlsFutureScores->GetStored()->insert(std::make_pair(toptHash, futureScore));
        }
      }
    }
  }

  void SetParameter(const std::string& key, const std::string& value) {
    if (key == "train") {
      m_train = Scan<bool>(value);
    } else if (key == "path") {
      m_modelPath = value;
    } else if (key == "vw-options") {
      m_vwOptions = value;
    } else if (key == "leave-one-out-from") {
      m_leaveOneOut = value;
    } else if (key == "training-loss") {
      // which type of loss to use for training
      if (value == "basic") {
        m_trainingLoss = (TrainingLoss *) new TrainingLossBasic();
      } else if (value == "bleu") {
        m_trainingLoss = (TrainingLoss *) new TrainingLossBLEU();
      } else {
        UTIL_THROW2("Unknown training loss type:" << value);
      }
    } else if (key == "loss") {
      // which normalizer to use (theoretically depends on the loss function used for training the
      // classifier (squared/logistic/hinge/...), hence the name "loss"
      if (value == "logistic") {
        m_normalizer = (Discriminative::Normalizer *) new Discriminative::LogisticLossNormalizer();
      } else if (value == "squared") {
        m_normalizer = (Discriminative::Normalizer *) new Discriminative::SquaredLossNormalizer();
      } else {
        UTIL_THROW2("Unknown loss type:" << value);
      }
    } else {
      StatefulFeatureFunction::SetParameter(key, value);
    }
  }

  virtual void InitializeForInput(ttasksptr const& ttask) {
    InputType const& source = *(ttask->GetSource().get());
    // tabbed sentence is assumed only in training
    if (! m_train)
      return;

    UTIL_THROW_IF2(source.GetType() != TabbedSentenceInput,
                   "This feature function requires the TabbedSentence input type");

    const TabbedSentence& tabbedSentence = static_cast<const TabbedSentence&>(source);
    UTIL_THROW_IF2(tabbedSentence.GetColumns().size() < 2,
                   "TabbedSentence must contain target<tab>alignment");

    // target sentence represented as a phrase
    Phrase *target = new Phrase();
    target->CreateFromString(
      Output
      , StaticData::Instance().options()->output.factor_order
      , tabbedSentence.GetColumns()[0]
      , NULL);

    // word alignment between source and target sentence
    // we don't store alignment info in AlignmentInfoCollection because we keep alignments of whole
    // sentences, not phrases
    AlignmentInfo *alignment = new AlignmentInfo(tabbedSentence.GetColumns()[1]);

    VWTargetSentence &targetSent = *GetStored();
    targetSent.Clear();
    targetSent.m_sentence = target;
    targetSent.m_alignment = alignment;

    // do not keep future cost estimates across sentences!
    m_tlsFutureScores->GetStored()->clear();

    // invalidate our caches after each sentence
    m_tlsComputedStateExtensions->GetStored()->clear();

    // pre-compute max- and min- aligned points for faster translation option checking
    targetSent.SetConstraints(source.GetSize());
  }


private:
  std::string MakeTargetLabel(const TargetPhrase &targetPhrase) const {
    return VW_DUMMY_LABEL;
  }

  std::pair<bool, int> IsCorrectTranslationOption(const TranslationOption &topt) const {

    //std::cerr << topt.GetSourceWordsRange() << std::endl;

    int sourceStart = topt.GetSourceWordsRange().GetStartPos();
    int sourceEnd   = topt.GetSourceWordsRange().GetEndPos();

    const VWTargetSentence &targetSentence = *GetStored();

    // [targetStart, targetEnd] spans aligned target words
    int targetStart = targetSentence.m_sentence->GetSize();
    int targetEnd   = -1;

    // get the left-most and right-most alignment point within source span
    for(int i = sourceStart; i <= sourceEnd; ++i) {
      if(targetSentence.m_sourceConstraints[i].IsSet()) {
        if(targetStart > targetSentence.m_sourceConstraints[i].GetMin())
          targetStart = targetSentence.m_sourceConstraints[i].GetMin();
        if(targetEnd < targetSentence.m_sourceConstraints[i].GetMax())
          targetEnd = targetSentence.m_sourceConstraints[i].GetMax();
      }
    }
    // there was no alignment
    if(targetEnd == -1)
      return std::make_pair(false, -1);

    //std::cerr << "Shorter: " << targetStart << " " << targetEnd << std::endl;

    // [targetStart2, targetEnd2] spans unaligned words left and right of [targetStart, targetEnd]
    int targetStart2 = targetStart;
    for(int i = targetStart2; i >= 0 && !targetSentence.m_targetConstraints[i].IsSet(); --i)
      targetStart2 = i;

    int targetEnd2   = targetEnd;
    for(int i = targetEnd2;
        i < targetSentence.m_sentence->GetSize() && !targetSentence.m_targetConstraints[i].IsSet();
        ++i)
      targetEnd2 = i;

    //std::cerr << "Longer: " << targetStart2 << " " << targetEnd2 << std::endl;

    const TargetPhrase &tphrase = topt.GetTargetPhrase();
    //std::cerr << tphrase << std::endl;

    // if target phrase is shorter than inner span return false
    if(tphrase.GetSize() < targetEnd - targetStart + 1)
      return std::make_pair(false, -1);

    // if target phrase is longer than outer span return false
    if(tphrase.GetSize() > targetEnd2 - targetStart2 + 1)
      return std::make_pair(false, -1);

    // for each possible starting point
    for(int tempStart = targetStart2; tempStart <= targetStart; tempStart++) {
      bool found = true;
      // check if the target phrase is within longer span
      for(int i = tempStart; i <= targetEnd2 && i < tphrase.GetSize() + tempStart; ++i) {
        if(tphrase.GetWord(i - tempStart) != targetSentence.m_sentence->GetWord(i)) {
          found = false;
          break;
        }
      }
      // return true if there was a match
      if(found) {
        //std::cerr << "Found" << std::endl;
        return std::make_pair(true, tempStart);
      }
    }

    return std::make_pair(false, -1);
  }

  std::vector<bool> LeaveOneOut(const TranslationOptionList &topts, const std::vector<bool> &correct) const {
    UTIL_THROW_IF2(m_leaveOneOut.size() == 0 || ! m_train, "LeaveOneOut called in wrong setting!");

    float sourceRawCount = 0.0;
    const float ONE = 1.0001; // I don't understand floating point numbers

    std::vector<bool> keepOpt;

    for (size_t i = 0; i < topts.size(); i++) {
      TranslationOption *topt = *(topts.begin() + i);
      const TargetPhrase &targetPhrase = topt->GetTargetPhrase();

      // extract raw counts from phrase-table property
      const CountsPhraseProperty *property =
        static_cast<const CountsPhraseProperty *>(targetPhrase.GetProperty("Counts"));

      if (! property) {
        VERBOSE(1, "VW :: Counts not found for topt! Is this an OOV?\n");
        // keep all translation opts without updating, this is either OOV or bad usage...
        keepOpt.assign(topts.size(), true);
        return keepOpt;
      }

      if (sourceRawCount == 0.0) {
        sourceRawCount = property->GetSourceMarginal() - ONE; // discount one occurrence of the source phrase
        if (sourceRawCount <= 0) {
          // no translation options survived, source phrase was a singleton
          keepOpt.assign(topts.size(), false);
          return keepOpt;
        }
      }

      float discount = correct[i] ? ONE : 0.0;
      float target = property->GetTargetMarginal() - discount;
      float joint  = property->GetJointCount() - discount;
      if (discount != 0.0) VERBOSE(2, "VW :: leaving one out!\n");

      if (joint > 0) {
        // topt survived leaving one out, update its scores
        const FeatureFunction *feature = &FindFeatureFunction(m_leaveOneOut);
        std::vector<float> scores = targetPhrase.GetScoreBreakdown().GetScoresForProducer(feature);
        UTIL_THROW_IF2(scores.size() != 4, "Unexpected number of scores in feature " << m_leaveOneOut);
        scores[0] = TransformScore(joint / target); // P(f|e)
        scores[2] = TransformScore(joint / sourceRawCount); // P(e|f)

        ScoreComponentCollection &scoreBreakDown = topt->GetScoreBreakdown();
        scoreBreakDown.Assign(feature, scores);
        topt->UpdateScore();
        keepOpt.push_back(true);
      } else {
        // they only occurred together once, discard topt
        VERBOSE(2, "VW :: discarded topt when leaving one out\n");
        keepOpt.push_back(false);
      }
    }

    return keepOpt;
  }

  inline size_t MakeCacheKey(const FFState *prevState, size_t spanStart, size_t spanEnd) const {
    size_t key = 0;
    boost::hash_combine(key, prevState);
    boost::hash_combine(key, spanStart);
    boost::hash_combine(key, spanEnd);
    return key;
  }

  // shift words in our state, add words from current hypothesis
  VWState *UpdateState(const FFState *prevState, const Hypothesis &curHypo) const {
    const VWState *prevVWState = static_cast<const VWState *>(prevState);

    VERBOSE(3, "VW :: updating state\n>> previous state: " << *prevVWState << "\n");

    // copy phrase from previous state
    Phrase phrase = prevVWState->m_phrase;
    size_t contextSize = phrase.GetSize(); // identical to VWFeatureBase::GetMaximumContextSize()
    
    // add words from current hypothesis
    phrase.Append(curHypo.GetCurrTargetPhrase());

    VERBOSE(3, ">> current hypo: " << curHypo.GetCurrTargetPhrase() << "\n");

    // get a slice of appropriate length
    Range range(phrase.GetSize() - contextSize, phrase.GetSize() - 1);
    phrase = phrase.GetSubString(range);

    // build the new state
    VWState *out = new VWState();
    out->m_phrase = phrase;

    VERBOSE(3, ">> updated state: " << *out << "\n");

    return out;
  }

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
};

}


#pragma once

#include <string>
#include <map>
#include <limits>

#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/PP/CountsPhraseProperty.h"
#include "moses/TranslationOptionList.h"
#include "moses/TranslationOption.h"
#include "moses/Util.h"
#include "moses/TypeDef.h"
#include "moses/StaticData.h"
#include "moses/Phrase.h"
#include "moses/AlignmentInfo.h"

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

typedef ThreadLocalByFeatureStorage<Discriminative::Classifier, Discriminative::ClassifierFactory &> TLSClassifier;

typedef ThreadLocalByFeatureStorage<VWTargetSentence> TLSTargetSentence;

class VW : public StatelessFeatureFunction, public TLSTargetSentence
{
public:
  VW(const std::string &line)
    : StatelessFeatureFunction(1, line)
    , TLSTargetSentence(this)
    , m_train(false) {
    ReadParameters();
    Discriminative::ClassifierFactory *classifierFactory = m_train
        ? new Discriminative::ClassifierFactory(m_modelPath)
        : new Discriminative::ClassifierFactory(m_modelPath, m_vwOptions);

    m_tlsClassifier = new TLSClassifier(this, *classifierFactory);

    if (! m_normalizer) {
      VERBOSE(1, "VW :: No loss function specified, assuming logistic loss.\n");
      m_normalizer = (Discriminative::Normalizer *) new Discriminative::LogisticLossNormalizer();
    }

    if (! m_trainingLoss) {
      VERBOSE(1, "VW :: Using basic 1/0 loss calculation in training.\n");
      m_trainingLoss = (TrainingLoss *) new TrainingLossBasic();
    }
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

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
    Discriminative::Classifier &classifier = *m_tlsClassifier->GetStored();

    if (translationOptionList.size() == 0)
      return; // nothing to do

    VERBOSE(2, "VW :: Evaluating translation options\n");

    // which feature functions do we use (on the source and target side)
    const std::vector<VWFeatureBase*>& sourceFeatures =
      VWFeatureBase::GetSourceFeatures(GetScoreProducerDescription());

    const std::vector<VWFeatureBase*>& targetFeatures =
      VWFeatureBase::GetTargetFeatures(GetScoreProducerDescription());

    const Range &sourceRange = translationOptionList.Get(0)->GetSourceWordsRange();
    const InputPath  &inputPath   = translationOptionList.Get(0)->GetInputPath();

    if (m_train) {
      //
      // extract features for training the classifier (only call this when using vwtrainer, not in Moses!)
      //

      // find which topts are correct
      std::vector<bool> correct(translationOptionList.size());
      for (size_t i = 0; i < translationOptionList.size(); i++)
        correct[i] = IsCorrectTranslationOption(* translationOptionList.Get(i));

      // optionally update translation options using leave-one-out
      std::vector<bool> keep = (m_leaveOneOut.size() > 0)
                               ? LeaveOneOut(translationOptionList, correct)
                               : std::vector<bool>(translationOptionList.size(), true);

      // check whether we (still) have some correct translation
      int firstCorrect = -1;
      for (size_t i = 0; i < translationOptionList.size(); i++) {
        if (keep[i] && correct[i]) {
          firstCorrect = i;
          break;
        }
      }

      // do not train if there are no positive examples
      if (firstCorrect == -1) {
        VERBOSE(2, "VW :: skipping topt collection, no correct translation for span\n");
        return;
      }

      // the first correct topt can be used by some loss functions
      const TargetPhrase &correctPhrase = translationOptionList.Get(firstCorrect)->GetTargetPhrase();

      // extract source side features
      for(size_t i = 0; i < sourceFeatures.size(); ++i)
        (*sourceFeatures[i])(input, inputPath, sourceRange, classifier);

      // go over topts, extract target side features and train the classifier
      for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {

        // this topt was discarded by leaving one out
        if (! keep[toptIdx])
          continue;

        // extract target-side features for each topt
        const TargetPhrase &targetPhrase = translationOptionList.Get(toptIdx)->GetTargetPhrase();
        for(size_t i = 0; i < targetFeatures.size(); ++i)
          (*targetFeatures[i])(input, inputPath, targetPhrase, classifier);

        float loss = (*m_trainingLoss)(targetPhrase, correctPhrase, correct[toptIdx]);

        // train classifier on current example
        classifier.Train(MakeTargetLabel(targetPhrase), loss);
      }
    } else {
      //
      // predict using a trained classifier, use this in decoding (=at test time)
      //

      std::vector<float> losses(translationOptionList.size());

      // extract source side features
      for(size_t i = 0; i < sourceFeatures.size(); ++i)
        (*sourceFeatures[i])(input, inputPath, sourceRange, classifier);

      for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {
        const TranslationOption *topt = translationOptionList.Get(toptIdx);
        const TargetPhrase &targetPhrase = topt->GetTargetPhrase();

        // extract target-side features for each topt
        for(size_t i = 0; i < targetFeatures.size(); ++i)
          (*targetFeatures[i])(input, inputPath, targetPhrase, classifier);

        // get classifier score
        losses[toptIdx] = classifier.Predict(MakeTargetLabel(targetPhrase));
      }

      // normalize classifier scores to get a probability distribution
      (*m_normalizer)(losses);

      // update scores of topts
      for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {
        TranslationOption *topt = *(translationOptionList.begin() + toptIdx);
        std::vector<float> newScores(m_numScoreComponents);
        newScores[0] = FloorScore(TransformScore(losses[toptIdx]));

        ScoreComponentCollection &scoreBreakDown = topt->GetScoreBreakdown();
        scoreBreakDown.PlusEquals(this, newScores);

        topt->UpdateScore();
      }
    }
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }

  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
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
      StatelessFeatureFunction::SetParameter(key, value);
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
      , StaticData::Instance().options().output.factor_order
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

    // pre-compute max- and min- aligned points for faster translation option checking
    targetSent.SetConstraints(source.GetSize());
  }


private:
  std::string MakeTargetLabel(const TargetPhrase &targetPhrase) const {
    return VW_DUMMY_LABEL;
  }

  bool IsCorrectTranslationOption(const TranslationOption &topt) const {

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
      return false;

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
      return false;

    // if target phrase is longer than outer span return false
    if(tphrase.GetSize() > targetEnd2 - targetStart2 + 1)
      return false;

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
        return true;
      }
    }

    return false;
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

  bool m_train; // false means predict
  std::string m_modelPath;
  std::string m_vwOptions;

  // calculator of training loss
  TrainingLoss *m_trainingLoss = NULL;

  // optionally contains feature name of a phrase table where we recompute scores with leaving one out
  std::string m_leaveOneOut;

  Discriminative::Normalizer *m_normalizer = NULL;
  TLSClassifier *m_tlsClassifier;
};

}


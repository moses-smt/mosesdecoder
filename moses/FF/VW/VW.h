#pragma once

#include <string>
#include <map>

#include "moses/FF/StatelessFeatureFunction.h"
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

namespace Moses
{

const std::string VW_DUMMY_LABEL = "1111"; // VW does not use the actual label, other classifiers might

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

  Phrase *m_sentence;
  AlignmentInfo *m_alignment;
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

    const std::vector<VWFeatureBase*>& sourceFeatures = VWFeatureBase::GetSourceFeatures(GetScoreProducerDescription());

    const WordsRange &sourceRange = translationOptionList.Get(0)->GetSourceWordsRange();
    const InputPath  &inputPath   = translationOptionList.Get(0)->GetInputPath();

    for(size_t i = 0; i < sourceFeatures.size(); ++i)
      (*sourceFeatures[i])(input, inputPath, sourceRange, classifier);

    const std::vector<VWFeatureBase*>& targetFeatures = VWFeatureBase::GetTargetFeatures(GetScoreProducerDescription());

    std::vector<float> losses(translationOptionList.size());

    std::vector<float>::iterator iterLoss;
    TranslationOptionList::const_iterator iterTransOpt;
    for(iterTransOpt = translationOptionList.begin(), iterLoss = losses.begin() ;
        iterTransOpt != translationOptionList.end() ; ++iterTransOpt, ++iterLoss) {

      const TargetPhrase &targetPhrase = (*iterTransOpt)->GetTargetPhrase();
      for(size_t i = 0; i < targetFeatures.size(); ++i)
        (*targetFeatures[i])(input, inputPath, targetPhrase, classifier);

      if (! m_train) {
        *iterLoss = classifier.Predict(MakeTargetLabel(targetPhrase));
      } else {
        float loss = IsCorrectTranslationOption(**iterTransOpt) ? 0.0 : 1.0;
        classifier.Train(MakeTargetLabel(targetPhrase), loss);
      }
    }

    (*m_normalizer)(losses);

    for(iterTransOpt = translationOptionList.begin(), iterLoss = losses.begin() ;
        iterTransOpt != translationOptionList.end() ; ++iterTransOpt, ++iterLoss) {
      TranslationOption &transOpt = **iterTransOpt;

      std::vector<float> newScores(m_numScoreComponents);
      newScores[0] = FloorScore(TransformScore(*iterLoss));

      ScoreComponentCollection &scoreBreakDown = transOpt.GetScoreBreakdown();
      scoreBreakDown.PlusEquals(this, newScores);

      transOpt.UpdateScore();
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
    } else if (key == "loss") {
      m_normalizer = value == "logistic"
                     ? (Discriminative::Normalizer *) new Discriminative::LogisticLossNormalizer()
                     : (Discriminative::Normalizer *) new Discriminative::SquaredLossNormalizer();
    } else {
      StatelessFeatureFunction::SetParameter(key, value);
    }
  }

  virtual void InitializeForInput(InputType const& source) {
    // tabbed sentence is assumed only in training
    if (! m_train)
      return;

    UTIL_THROW_IF2(source.GetType() != TabbedSentenceInput, "This feature function requires the TabbedSentence input type");

    const TabbedSentence& tabbedSentence = static_cast<const TabbedSentence&>(source);
    UTIL_THROW_IF2(tabbedSentence.GetColumns().size() < 2, "TabbedSentence must contain target<tab>alignment");

    // target sentence represented as a phrase
    Phrase *target = new Phrase();
    target->CreateFromString(
      Output
      , StaticData::Instance().GetOutputFactorOrder()
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
  }


private:
  std::string MakeTargetLabel(const TargetPhrase &targetPhrase) const {
    return VW_DUMMY_LABEL;
  }

  bool IsCorrectTranslationOption(const TranslationOption &topt) const {
    size_t sourceStart = topt.GetSourceWordsRange().GetStartPos();
    size_t sourceEnd   = topt.GetSourceWordsRange().GetEndPos() + 1;

    const VWTargetSentence &targetSentence = *GetStored();

    // get the left-most alignment point withitn sourceRange
    std::set<size_t> aligned;
    while ((aligned = targetSentence.m_alignment->GetAlignmentsForSource(sourceStart)).empty()) {
      sourceStart++;

      if (sourceStart >= sourceEnd) {
        // no alignment point between source and target sentence within current source span;
        // return immediately
        return false;
      }
    }

    size_t targetSentOffset = *aligned.begin(); // index of first aligned target word covered in source span

    const TargetPhrase &tphrase = topt.GetTargetPhrase();

    // get the left-most alignment point within topt
    size_t targetStart = 0;
    while ((aligned = tphrase.GetAlignTerm().GetAlignmentsForSource(targetStart)).empty())
      targetStart++;

    size_t toptOffset = *aligned.begin(); // index of first aligned target word in the translation option

    size_t startAt = targetSentOffset - toptOffset;
    bool matches = true;
    for (size_t i = 0; i < tphrase.GetSize(); i++) {
      if (startAt + i >= targetSentence.m_sentence->GetSize()
          || tphrase.GetWord(i) != targetSentence.m_sentence->GetWord(startAt + i)) {
        matches = false;
        break;
      }
    }

    return matches;
  }

  bool m_train; // false means predict
  std::string m_modelPath;
  std::string m_vwOptions;
  Discriminative::Normalizer *m_normalizer = NULL;
  TLSClassifier *m_tlsClassifier;
};

}


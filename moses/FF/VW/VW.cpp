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
#include "moses/AlignmentInfoCollection.h"
#include "moses/Word.h"
#include "moses/FactorCollection.h"

#include "Normalizer.h"
#include "Classifier.h"
#include "VWFeatureBase.h"
#include "TabbedSentence.h"
#include "ThreadLocalByFeatureStorage.h"
#include "TrainingLoss.h"
#include "VWTargetSentence.h"
#include "VWState.h"
#include "VW.h"

namespace Moses
{

VW::VW(const std::string &line)
  : StatefulFeatureFunction(1, line)
  , TLSTargetSentence(this)
  , m_train(false)
  , m_sentenceStartWord(Word())
{
  ReadParameters();
  Discriminative::ClassifierFactory *classifierFactory = m_train
      ? new Discriminative::ClassifierFactory(m_modelPath)
      : new Discriminative::ClassifierFactory(m_modelPath, m_vwOptions);

  m_tlsClassifier = new TLSClassifier(this, *classifierFactory);

  m_tlsFutureScores = new TLSFloatHashMap(this);
  m_tlsComputedStateExtensions = new TLSStateExtensions(this);
  m_tlsTranslationOptionFeatures = new TLSFeatureVectorMap(this);
  m_tlsTargetContextFeatures = new TLSFeatureVectorMap(this);

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

VW::~VW()
{
  delete m_tlsClassifier;
  delete m_normalizer;
  // TODO delete more stuff
}

FFState* VW::EvaluateWhenApplied(
  const Hypothesis& curHypo,
  const FFState* prevState,
  ScoreComponentCollection* accumulator) const
{
  VERBOSE(3, "VW :: Evaluating translation options\n");

  const VWState& prevVWState = *static_cast<const VWState *>(prevState);

  const std::vector<VWFeatureBase*>& contextFeatures =
    VWFeatureBase::GetTargetContextFeatures(GetScoreProducerDescription());

  if (contextFeatures.empty()) {
    // no target context features => we already evaluated everything in
    // EvaluateTranslationOptionListWithSourceContext(). Nothing to do now,
    // no state information to track.
    return new VWState();
  }

  size_t spanStart = curHypo.GetTranslationOption().GetStartPos();
  size_t spanEnd   = curHypo.GetTranslationOption().GetEndPos();

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

    // extract target context features
    size_t contextHash = prevVWState.hash();

    FeatureVectorMap &contextFeaturesCache = *m_tlsTargetContextFeatures->GetStored();

    FeatureVectorMap::const_iterator contextIt = contextFeaturesCache.find(contextHash);
    if (contextIt == contextFeaturesCache.end()) {
      // we have not extracted features for this context yet

      const Phrase &targetContext = prevVWState.GetPhrase();
      Discriminative::FeatureVector contextVector;
      const AlignmentInfo *alignInfo = TransformAlignmentInfo(curHypo, targetContext.GetSize());
      for(size_t i = 0; i < contextFeatures.size(); ++i)
        (*contextFeatures[i])(input, targetContext, *alignInfo, classifier, contextVector);

      contextFeaturesCache[contextHash] = contextVector;
      VERBOSE(3, "VW :: context cache miss\n");
    } else {
      // context already in cache, simply put feature IDs in the classifier object
      classifier.AddLabelIndependentFeatureVector(contextIt->second);
      VERBOSE(3, "VW :: context cache hit\n");
    }

    std::vector<float> losses(topts->size());

    for (size_t toptIdx = 0; toptIdx < topts->size(); toptIdx++) {
      const TranslationOption *topt = topts->Get(toptIdx);
      const TargetPhrase &targetPhrase = topt->GetTargetPhrase();
      size_t toptHash = hash_value(*topt);

      // start with pre-computed source-context-only VW scores
      losses[toptIdx] = m_tlsFutureScores->GetStored()->find(toptHash)->second;

      // add all features associated with this translation option
      // (pre-computed when evaluated with source context)
      const Discriminative::FeatureVector &targetFeatureVector =
        m_tlsTranslationOptionFeatures->GetStored()->find(toptHash)->second;

      classifier.AddLabelDependentFeatureVector(targetFeatureVector);

      // add classifier score with context+target features only to the total loss
      losses[toptIdx] += classifier.Predict(MakeTargetLabel(targetPhrase));
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

  return new VWState(prevVWState, curHypo);
}

const FFState* VW::EmptyHypothesisState(const InputType &input) const
{
  size_t maxContextSize = VWFeatureBase::GetMaximumContextSize(GetScoreProducerDescription());
  Phrase initialPhrase;
  for (size_t i = 0; i < maxContextSize; i++)
    initialPhrase.AddWord(m_sentenceStartWord);

  return new VWState(initialPhrase);
}

void VW::EvaluateTranslationOptionListWithSourceContext(const InputType &input
    , const TranslationOptionList &translationOptionList) const
{
  Discriminative::Classifier &classifier = *m_tlsClassifier->GetStored();

  if (translationOptionList.size() == 0)
    return; // nothing to do

  VERBOSE(3, "VW :: Evaluating translation options\n");

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
        VERBOSE(3, "VW :: skipping topt collection, no correct translation for span at current tgt start position\n");
        continue;
      }

      // the first correct topt can be used by some loss functions
      const TargetPhrase &correctPhrase = translationOptionList.Get(firstCorrect)->GetTargetPhrase();

      // feature extraction *at prediction time* outputs feature hashes which can be cached;
      // this is training time, simply store everything in this dummyVector
      Discriminative::FeatureVector dummyVector;

      // extract source side features
      for(size_t i = 0; i < sourceFeatures.size(); ++i)
        (*sourceFeatures[i])(input, sourceRange, classifier, dummyVector);

      // build target-side context
      Phrase targetContext;
      for (size_t i = 0; i < maxContextSize; i++)
        targetContext.AddWord(m_sentenceStartWord);

      const Phrase *targetSent = GetStored()->m_sentence;

      // word alignment info shifted by context size
      AlignmentInfo contextAlignment = TransformAlignmentInfo(*GetStored()->m_alignment, maxContextSize, currentStart);

      if (currentStart > 0)
        targetContext.Append(targetSent->GetSubString(Range(0, currentStart - 1)));

      // extract target-context features
      for(size_t i = 0; i < contextFeatures.size(); ++i)
        (*contextFeatures[i])(input, targetContext, contextAlignment, classifier, dummyVector);

      // go over topts, extract target side features and train the classifier
      for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {

        // this topt was discarded by leaving one out
        if (! keep[toptIdx])
          continue;

        // extract target-side features for each topt
        const TargetPhrase &targetPhrase = translationOptionList.Get(toptIdx)->GetTargetPhrase();
        for(size_t i = 0; i < targetFeatures.size(); ++i)
          (*targetFeatures[i])(input, targetPhrase, classifier, dummyVector);

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

    Discriminative::FeatureVector outFeaturesSourceNamespace;

    // extract source side features
    for(size_t i = 0; i < sourceFeatures.size(); ++i)
      (*sourceFeatures[i])(input, sourceRange, classifier, outFeaturesSourceNamespace);

    for (size_t toptIdx = 0; toptIdx < translationOptionList.size(); toptIdx++) {
      const TranslationOption *topt = translationOptionList.Get(toptIdx);
      const TargetPhrase &targetPhrase = topt->GetTargetPhrase();
      Discriminative::FeatureVector outFeaturesTargetNamespace;

      // extract target-side features for each topt
      for(size_t i = 0; i < targetFeatures.size(); ++i)
        (*targetFeatures[i])(input, targetPhrase, classifier, outFeaturesTargetNamespace);

      // cache the extracted target features (i.e. features associated with given topt)
      // for future use at decoding time
      size_t toptHash = hash_value(*topt);
      m_tlsTranslationOptionFeatures->GetStored()->insert(
        std::make_pair(toptHash, outFeaturesTargetNamespace));

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
        // We have target context features => this is just a partial score,
        // do not add it to the score component collection.
        size_t toptHash = hash_value(*topt);

        // Subtract the score contribution of target-only features, otherwise it would
        // be included twice.
        Discriminative::FeatureVector emptySource;
        const Discriminative::FeatureVector &targetFeatureVector =
          m_tlsTranslationOptionFeatures->GetStored()->find(toptHash)->second;
        classifier.AddLabelIndependentFeatureVector(emptySource);
        classifier.AddLabelDependentFeatureVector(targetFeatureVector);
        float targetOnlyLoss = classifier.Predict(VW_DUMMY_LABEL);

        float futureScore = rawLosses[toptIdx] - targetOnlyLoss;
        m_tlsFutureScores->GetStored()->insert(std::make_pair(toptHash, futureScore));
      }
    }
  }
}

void VW::SetParameter(const std::string& key, const std::string& value)
{
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

void VW::InitializeForInput(ttasksptr const& ttask)
{
  // do not keep future cost estimates across sentences!
  m_tlsFutureScores->GetStored()->clear();

  // invalidate our caches after each sentence
  m_tlsComputedStateExtensions->GetStored()->clear();

  // it's not certain that we should clear these caches; we do it
  // because they shouldn't be allowed to grow indefinitely large but
  // target contexts and translation options will have identical features
  // the next time we extract them...
  m_tlsTargetContextFeatures->GetStored()->clear();
  m_tlsTranslationOptionFeatures->GetStored()->clear();

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

  // pre-compute max- and min- aligned points for faster translation option checking
  targetSent.SetConstraints(source.GetSize());
}

/*************************************************************************************
 * private methods
 ************************************************************************************/

const AlignmentInfo *VW::TransformAlignmentInfo(const Hypothesis &curHypo, size_t contextSize) const
{
  std::set<std::pair<size_t, size_t> > alignmentPoints;
  const Hypothesis *contextHypo = curHypo.GetPrevHypo();
  int idxInContext = contextSize - 1;
  int processedWordsInHypo = 0;
  while (idxInContext >= 0 && contextHypo) {
    int idxInHypo = contextHypo->GetCurrTargetLength() - 1 - processedWordsInHypo;
    if (idxInHypo >= 0) {
      const AlignmentInfo &hypoAlign = contextHypo->GetCurrTargetPhrase().GetAlignTerm();
      std::set<size_t> alignedToTgt = hypoAlign.GetAlignmentsForTarget(idxInHypo);
      size_t srcOffset = contextHypo->GetCurrSourceWordsRange().GetStartPos();
      BOOST_FOREACH(size_t srcIdx, alignedToTgt) {
        alignmentPoints.insert(std::make_pair(srcOffset + srcIdx, idxInContext));
      }
      processedWordsInHypo++;
      idxInContext--;
    } else {
      processedWordsInHypo = 0;
      contextHypo = contextHypo->GetPrevHypo();
    }
  }

  return AlignmentInfoCollection::Instance().Add(alignmentPoints);
}

AlignmentInfo VW::TransformAlignmentInfo(const AlignmentInfo &alignInfo, size_t contextSize, int currentStart) const
{
  std::set<std::pair<size_t, size_t> > alignmentPoints;
  for (int i = std::max(0, currentStart - (int)contextSize); i < currentStart; i++) {
    std::set<size_t> alignedToTgt = alignInfo.GetAlignmentsForTarget(i);
    BOOST_FOREACH(size_t srcIdx, alignedToTgt) {
      alignmentPoints.insert(std::make_pair(srcIdx, i + contextSize));
    }
  }
  return AlignmentInfo(alignmentPoints);
}

std::pair<bool, int> VW::IsCorrectTranslationOption(const TranslationOption &topt) const
{

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

std::vector<bool> VW::LeaveOneOut(const TranslationOptionList &topts, const std::vector<bool> &correct) const
{
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
      VERBOSE(2, "VW :: Counts not found for topt! Is this an OOV?\n");
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
    if (discount != 0.0) VERBOSE(3, "VW :: leaving one out!\n");

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

} // namespace Moses

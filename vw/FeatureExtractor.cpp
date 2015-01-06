#include "FeatureExtractor.h"
#include "Util.h"

#include <algorithm>
#include <set>

using namespace std;
using namespace Moses;

namespace Discriminative
{

FeatureExtractor::FeatureExtractor(const ExtractorConfig &config, bool train)
  : m_config(config), m_train(train)
{  
  if (! m_config.IsLoaded())
    throw logic_error("configuration file not loaded");
}

void FeatureExtractor::GenerateFeatures(Classifier *fc,
  const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  const vector<Translation> &translations,
  vector<float> &losses)
{  
  fc->SetNamespace('s', true);

  if (m_config.GetSourceExternal()) GenerateContextFeatures(context, spanStart, spanEnd, fc);

  // get words (surface forms) in source phrase
  vector<string> sourceForms(spanEnd - spanStart + 1);
  for (size_t i = spanStart; i <= spanEnd; i++)
    sourceForms[i - spanStart] = context[i][FACTOR_FORM]; 
  
  map<string, float> maxProbs;
  if (m_config.GetMostFrequent()) maxProbs = GetMaxProb(translations);

  if (m_config.GetSourceInternal()) GenerateInternalFeatures(sourceForms, fc);
  if (m_config.GetPhraseFactor()) GeneratePhraseFactorFeatures(context, spanStart, spanEnd, fc);
  if (m_config.GetBagOfWords()) GenerateBagOfWordsFeatures(context, spanStart, spanEnd, FACTOR_FORM, fc);

	if (m_config.GetSourceIndicator()) GenerateIndicatorFeature(sourceForms, fc); 

  vector<Translation>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    fc->SetNamespace('t', false);

    // get words in target phrase
    const vector<string> &targetForms = transIt->translation;

    if (m_config.GetTargetInternal()) GenerateInternalFeatures(targetForms, fc);
    if (m_config.GetPaired()) GeneratePairedFeatures(sourceForms, targetForms, transIt->m_alignment, fc);

    if (m_config.GetMostFrequent()) GenerateMostFrequentFeature(transIt->m_ttableScores, maxProbs, fc);

    if (m_config.GetBinnedScores()) GenerateScoreFeatures(transIt->m_ttableScores, fc);

    // "NOT_IN_" features
    if (m_config.GetBinnedScores() || m_config.GetMostFrequent()) GenerateTTableEntryFeatures(transIt->m_ttableScores, fc);

		if (m_config.GetTargetIndicator()) GenerateIndicatorFeature(targetForms, fc); 

		if (m_config.GetSourceTargetIndicator()) GenerateConcatIndicatorFeature(sourceForms, targetForms, fc); 

		if (m_config.GetSTSE()) GenerateSTSE(sourceForms, targetForms, context, spanStart, spanEnd, fc); 

    if (m_train) {
      fc->Train(SPrint(DUMMY_IDX), *lossIt);
    } else {
      *lossIt = fc->Predict(SPrint(DUMMY_IDX));
    }
  }
  fc->FinishExample();
}

//
// private methods
//

string FeatureExtractor::BuildContextFeature(size_t factor, int index, const string &value)
{
  return "c^" + SPrint(factor) + "_" + SPrint(index) + "_" + value;
}

void FeatureExtractor::GenerateContextFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  Classifier *fc)
{
  vector<size_t>::const_iterator factIt;
  for (factIt = m_config.GetFactors().begin(); factIt != m_config.GetFactors().end(); factIt++) {
    for (size_t i = 1; i <= m_config.GetWindowSize(); i++) {
      string left = "<s>";
      string right = "</s>"; 
      if (spanStart >= i)
        left = context[spanStart - i][*factIt];
      fc->AddFeature(BuildContextFeature(*factIt, -i, left));
      if (spanEnd + i < context.size()) 
        right = context[spanEnd + i][*factIt];
      fc->AddFeature(BuildContextFeature(*factIt, i, right));
    }
  }
}

void FeatureExtractor::GenerateIndicatorFeature(const vector<string> &span, Classifier *fc)
{
  fc->AddFeature("p^" + Join("_", span));
}

void FeatureExtractor::GenerateConcatIndicatorFeature(const vector<string> &span1, const vector<string> &span2, Classifier *fc)
{
  fc->AddFeature("p^" + Join("_", span1) + "^" + Join("_", span2));
}

void FeatureExtractor::GenerateSTSE(const vector<string> &span1, const vector<string> &span2, 
  const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  Classifier *fc)
{
  vector<size_t>::const_iterator factIt;
  for (factIt = m_config.GetFactors().begin(); factIt != m_config.GetFactors().end(); factIt++) {
    for (size_t i = 1; i <= m_config.GetWindowSize(); i++) {
      string left = "<s>";
      string right = "</s>"; 
      if (spanStart >= i)
        left = context[spanStart - i][*factIt];
      fc->AddFeature("stse^" + Join("_", span1) + "^" + Join("_", span2) + BuildContextFeature(*factIt, -i, left));
      if (spanEnd + i < context.size()) 
        right = context[spanEnd + i][*factIt];
      fc->AddFeature("stse^" + Join("_", span1) + "^" + Join("_", span2) + BuildContextFeature(*factIt, i, right));
    }
  }
}

void FeatureExtractor::GenerateInternalFeatures(const vector<string> &span, Classifier *fc)
{
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    fc->AddFeature("w^" + *it);
  }
}

void FeatureExtractor::GenerateBagOfWordsFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, size_t factorID, Classifier *fc)
{
  for (size_t i = 0; i < spanStart; i++)
    fc->AddFeature("bow^" + context[i][factorID]);
  for (size_t i = spanEnd + 1; i < context.size(); i++)
    fc->AddFeature("bow^" + context[i][factorID]);
}

void FeatureExtractor::GeneratePhraseFactorFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, Classifier *fc)
{
  for (size_t i = spanStart; i <= spanEnd; i++) {
    vector<size_t>::const_iterator factIt;
    for (factIt = m_config.GetFactors().begin(); factIt != m_config.GetFactors().end(); factIt++) {
      fc->AddFeature("ibow^" + SPrint(*factIt) + "_" + context[i][*factIt]);
    }
  }
}

void FeatureExtractor::GeneratePairedFeatures(const vector<string> &srcPhrase, const vector<string> &tgtPhrase, 
    const AlignmentType &align, Classifier *fc)
{
  AlignmentType::const_iterator it;
  set<size_t> srcAligned;
  set<size_t> tgtAligned;

  for (it = align.begin(); it != align.end(); it++) {
    fc->AddFeature("pair^" + srcPhrase[it->first] + "^" + tgtPhrase[it->second]);
    srcAligned.insert(it->first);
    tgtAligned.insert(it->second);
  }

  for (size_t i = 0; i < srcPhrase.size(); i++) {
    if (srcAligned.count(i) == 0)
      fc->AddFeature("pair^" + srcPhrase[i] + "^NULL");
  }

  for (size_t i = 0; i < tgtPhrase.size(); i++) {
    if (tgtAligned.count(i) == 0)
      fc->AddFeature("pair^NULL^" + tgtPhrase[i]);
  }
}

void FeatureExtractor::GenerateScoreFeatures(const std::vector<TTableEntry> &ttableScores, Classifier *fc)
{
  vector<size_t>::const_iterator scoreIt;
  vector<float>::const_iterator binIt;
  vector<TTableEntry>::const_iterator tableIt;
  const vector<size_t> &scoreIDs = m_config.GetScoreIndexes();
  const vector<float> &bins = m_config.GetScoreBins();

  for (tableIt = ttableScores.begin(); tableIt != ttableScores.end(); tableIt++) {
    if (! tableIt->m_exists)
      continue;
    string prefix = ttableScores.size() == 1 ? "" : tableIt->m_id + "_";
    for (scoreIt = scoreIDs.begin(); scoreIt != scoreIDs.end(); scoreIt++) {
      for (binIt = bins.begin(); binIt != bins.end(); binIt++) {
        float logScore = log(tableIt->m_scores[*scoreIt]);
        if (logScore < *binIt || Equals(logScore, *binIt)) {
          fc->AddFeature(prefix + "sc^" + SPrint<size_t>(*scoreIt) + "_" + SPrint(*binIt));
        }
      }
    }
  }
}

void FeatureExtractor::GenerateMostFrequentFeature(const std::vector<TTableEntry> &ttableScores, const map<string, float> &maxProbs, Classifier *fc)
{
  vector<TTableEntry>::const_iterator it;
  for (it = ttableScores.begin(); it != ttableScores.end(); it++) {
    if (it->m_exists && Equals(it->m_scores[P_E_F_INDEX], maxProbs.find(it->m_id)->second)) {
      string prefix = ttableScores.size() == 1 ? "" : it->m_id + "_";
      fc->AddFeature(prefix + "MOST_FREQUENT");
    }
  }
}

void FeatureExtractor::GenerateTTableEntryFeatures(const std::vector<TTableEntry> &ttableScores, Classifier *fc)
{
  vector<TTableEntry>::const_iterator it;
  for (it = ttableScores.begin(); it != ttableScores.end(); it++) {
    if (! it->m_exists)
      fc->AddFeature("NOT_IN_" + it->m_id);
  }
}

} // namespace Discriminative

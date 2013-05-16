#include "FeatureExtractor.h"
#include "Util.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <set>

using namespace std;
using namespace boost::bimaps;
using namespace boost::property_tree;
using namespace Moses;

namespace PSD
{

FeatureExtractor::FeatureExtractor(const IndexType &targetIndex, const ExtractorConfig &config, bool train)
  : m_targetIndex(targetIndex), m_config(config), m_train(train)
{  
  if (! m_config.IsLoaded())
    throw logic_error("configuration file not loaded");
}

map<string, float> FeatureExtractor::GetMaxProb(const vector<Translation> &translations)
{
  map<string, float> maxProbs;
  vector<Translation>::const_iterator it;
  vector<TTableEntry>::const_iterator tableIt;
  for (it = translations.begin(); it != translations.end(); it++) {
    for (tableIt = it->m_ttableScores.begin(); tableIt != it->m_ttableScores.end(); tableIt++) {
      if (tableIt->m_exists) {
        maxProbs[tableIt->m_id] = max(tableIt->m_scores[P_E_F_INDEX], maxProbs[tableIt->m_id]);
      }
    }
  }
  return maxProbs;
}

void FeatureExtractor::GenerateFeatures(FeatureConsumer *fc,
  const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  const vector<Translation> &translations,
  vector<float> &losses,
  string extraFeature)
{  
  fc->SetNamespace('s', true);

  // XXX hack
  if (extraFeature != "")
    fc->AddFeature(extraFeature);

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
    vector<string> targetForms = Tokenize(m_targetIndex.right.find(transIt->m_index)->second, " ");
    // cerr << "Predicting score for phrase " << Join(" ", targetForms) << endl;

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
      fc->Train(SPrint(transIt->m_index), *lossIt);
    } else {
      *lossIt = fc->Predict(SPrint(transIt->m_index));
    }
  }
  fc->FinishExample();
}

ExtractorConfig::ExtractorConfig()
: m_paired(false), m_bagOfWords(false), m_sourceExternal(false),
    m_sourceInternal(false), m_targetInternal(false), m_windowSize(0)
{}

void ExtractorConfig::Load(const string &configFile)
{
  ptree pTree;
  ini_parser::read_ini(configFile, pTree);
  m_sourceInternal  = pTree.get<bool>("features.source-internal", false);
  m_sourceExternal  = pTree.get<bool>("features.source-external", false);
  m_targetInternal  = pTree.get<bool>("features.target-internal", false);
  m_sourceIndicator = pTree.get<bool>("features.source-indicator", false);
  m_targetIndicator = pTree.get<bool>("features.target-indicator", false);
  m_sourceTargetIndicator = pTree.get<bool>("features.source-target-indicator", false);
  m_STSE = pTree.get<bool>("features.source-target-source-external", false);
  m_paired          = pTree.get<bool>("features.paired", false);
  m_bagOfWords      = pTree.get<bool>("features.bag-of-words", false);
  m_mostFrequent    = pTree.get<bool>("features.most-frequent", false);
  m_binnedScores    = pTree.get<bool>("features.binned-scores", false);
  m_sourceTopic     = pTree.get<bool>("features.source-topic", false);
  m_phraseFactor    = pTree.get<bool>("features.phrase-factor", false);
  m_windowSize      = pTree.get<size_t>("features.window-size", 0);  

  m_factors = Scan<size_t>(Tokenize(pTree.get<string>("features.factors", ""), ","));
  m_scoreIndexes = Scan<size_t>(Tokenize(pTree.get<string>("features.scores", ""), ","));
  m_scoreBins = Scan<float>(Tokenize(pTree.get<string>("features.score-bins", ""), ","));

  m_vwOptsTrain = pTree.get<string>("vw-options.train", "");
  m_vwOptsPredict = pTree.get<string>("vw-options.predict", "");

  m_normalization = pTree.get<string>("decoder.normalization", "");

  m_isLoaded = true;
}

//
// private methods
//

string FeatureExtractor::BuildContextFeature(size_t factor, int index, const string &value)
{
  return "c^" + SPrint(factor) + "_" + SPrint(index) + "_" + value;
}

void FeatureExtractor::GenerateSourceTopicFeatures(const vector<string> &wordSpan, const vector<string> &sourceTopics, FeatureConsumer *fc)
{
//this grabs the words in the span of the current phrase
//next, adds topics values string for span
  vector<string>::const_iterator wordIt;
  vector<string>::const_iterator topicIt = sourceTopics.begin();
  for (wordIt = wordSpan.begin(); wordIt != wordSpan.end(); wordIt++, topicIt++)
    fc->AddFeature("srcTopic^" + *wordIt + "_" + *topicIt);
}

void FeatureExtractor::GenerateContextFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  FeatureConsumer *fc)
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

void FeatureExtractor::GenerateIndicatorFeature(const vector<string> &span, FeatureConsumer *fc)
{
  fc->AddFeature("p^" + Join("_", span));
}

void FeatureExtractor::GenerateConcatIndicatorFeature(const vector<string> &span1, const vector<string> &span2, FeatureConsumer *fc)
{
  fc->AddFeature("p^" + Join("_", span1) + "^" + Join("_", span2));
}

void FeatureExtractor::GenerateSTSE(const vector<string> &span1, const vector<string> &span2, 
  const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  FeatureConsumer *fc)
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

void FeatureExtractor::GenerateInternalFeatures(const vector<string> &span, FeatureConsumer *fc)
{
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    fc->AddFeature("w^" + *it);
  }
}

void FeatureExtractor::GenerateBagOfWordsFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, size_t factorID, FeatureConsumer *fc)
{
  for (size_t i = 0; i < spanStart; i++)
    fc->AddFeature("bow^" + context[i][factorID]);
  for (size_t i = spanEnd + 1; i < context.size(); i++)
    fc->AddFeature("bow^" + context[i][factorID]);
}

void FeatureExtractor::GeneratePhraseFactorFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc)
{
  for (size_t i = spanStart; i <= spanEnd; i++) {
    vector<size_t>::const_iterator factIt;
    for (factIt = m_config.GetFactors().begin(); factIt != m_config.GetFactors().end(); factIt++) {
      fc->AddFeature("ibow^" + SPrint(*factIt) + "_" + context[i][*factIt]);
    }
  }
}

void FeatureExtractor::GeneratePairedFeatures(const vector<string> &srcPhrase, const vector<string> &tgtPhrase, 
    const AlignmentType &align, FeatureConsumer *fc)
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

void FeatureExtractor::GenerateScoreFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc)
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

void FeatureExtractor::GenerateMostFrequentFeature(const std::vector<TTableEntry> &ttableScores, const map<string, float> &maxProbs, FeatureConsumer *fc)
{
  vector<TTableEntry>::const_iterator it;
  for (it = ttableScores.begin(); it != ttableScores.end(); it++) {
    if (it->m_exists && Equals(it->m_scores[P_E_F_INDEX], maxProbs.find(it->m_id)->second)) {
      string prefix = ttableScores.size() == 1 ? "" : it->m_id + "_";
      fc->AddFeature(prefix + "MOST_FREQUENT");
    }
  }
}

void FeatureExtractor::GenerateTTableEntryFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc)
{
  vector<TTableEntry>::const_iterator it;
  for (it = ttableScores.begin(); it != ttableScores.end(); it++) {
    if (! it->m_exists)
      fc->AddFeature("NOT_IN_" + it->m_id);
  }
}

} // namespace PSD

#include "FeatureExtractor.h"
#include "Util.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <exception>
#include <stdexcept>
#include <algorithm>

using namespace std;
using namespace boost::bimaps;
using namespace boost::property_tree;
using namespace Moses;

namespace PSD
{

FeatureExtractor::FeatureExtractor(const TargetIndexType &targetIndex, const ExtractorConfig &config, bool train)
  : m_targetIndex(targetIndex), m_config(config), m_train(train)
{  
  if (! m_config.IsLoaded())
    throw logic_error("configuration file not loaded");
}

float FeatureExtractor::GetMaxProb(const vector<Translation> &translations)
{
  float maxProb = 0; 
  vector<Translation>::const_iterator it;
  for (it = translations.begin(); it != translations.end(); it++) 
    maxProb = max(it->m_scores[P_E_F_INDEX], maxProb);
  return maxProb;
}

void FeatureExtractor::GenerateFeatures(FeatureConsumer *fc,
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
  
  float maxProb = 0;
  if (m_config.GetMostFrequent()) maxProb = GetMaxProb(translations);

  if (m_config.GetSourceInternal()) GenerateInternalFeatures(sourceForms, fc);
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

    if (m_config.GetMostFrequent() && Equals(transIt->m_scores[P_E_F_INDEX], maxProb)) 
      fc->AddFeature("MOST_FREQUENT");

    if (m_config.GetBinnedScores()) GenerateScoreFeatures(transIt->m_scores, fc);

		if (m_config.GetTargetIndicator()) GenerateIndicatorFeature(targetForms, fc); 

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
  m_paired          = pTree.get<bool>("features.paired", false);
  m_bagOfWords      = pTree.get<bool>("features.bag-of-words", false);
  m_mostFrequent    = pTree.get<bool>("features.most-frequent", false);
  m_binnedScores    = pTree.get<bool>("features.binned-scores", false);
  m_sourceTopic     = pTree.get<bool>("features.source-topic", false);
  m_windowSize      = pTree.get<size_t>("features.window-size", 0);  

  m_factors = Scan<size_t>(Tokenize(pTree.get<string>("features.factors", ""), ","));
  m_scoreIndexes = Scan<size_t>(Tokenize(pTree.get<string>("features.scores", ""), ","));

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
      if (spanStart >= i) 
        fc->AddFeature(BuildContextFeature(*factIt, -i, context[spanStart - i][*factIt]));
      if (spanEnd + i < context.size())
        fc->AddFeature(BuildContextFeature(*factIt, i, context[spanEnd + i][*factIt]));
    }
  }
}

void FeatureExtractor::GenerateIndicatorFeature(const vector<string> &span, FeatureConsumer *fc)
{
  fc->AddFeature("p^" + Join("_", span));
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

void FeatureExtractor::GeneratePairedFeatures(const vector<string> &srcPhrase, const vector<string> &tgtPhrase, 
    const AlignmentType &align, FeatureConsumer *fc)
{
  AlignmentType::const_iterator it;
  for (it = align.begin(); it != align.end(); it++)
    fc->AddFeature("pair^" + srcPhrase[it->first] + "^" + tgtPhrase[it->second]);
}

void FeatureExtractor::GenerateScoreFeatures(const std::vector<float> scores, FeatureConsumer *fc)
{
  vector<size_t>::const_iterator it;
  const vector<size_t>& scoreIDs = m_config.GetScoreIndexes();
  for (it = scoreIDs.begin(); it != scoreIDs.end(); it++)
    fc->AddFeature("sc^" + SPrint<size_t>(*it) + "_" + SPrint((int)log(scores[*it])));
}

} // namespace PSD

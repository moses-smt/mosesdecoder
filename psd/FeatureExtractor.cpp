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

float FeatureExtractor::GetMaxProbChart(const vector<ChartTranslation> &translations)
{
  float maxProb = 0;
  vector<ChartTranslation>::const_iterator it;
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

void FeatureExtractor::GenerateFeaturesChart(FeatureConsumer *fc,
  const ContextType &context,
  const std::string &sourceSide,
  //const vector<string> &sourceTopics,
  const vector<string> &syntaxLabels,
  const std::string parent,
  const std::string span,
  size_t spanStart,
  size_t spanEnd,
  const vector<ChartTranslation> &translations,
  vector<float> &losses)
{
  fc->SetNamespace('s', true);
  if (m_config.GetSourceExternal()) GenerateContextFeatures(context, spanStart, spanEnd, fc);


  // tokenize source side of rule
  vector<string> sourceForms;
  sourceForms = Tokenize(sourceSide, " ");

  float maxProb = 0;
  if (m_config.GetMostFrequent()) maxProb = GetMaxProbChart(translations);

  //vector<string> phraseTopics(sourceTopics.begin() + spanStart, sourceTopics.begin() + spanEnd + 1);

  //if (m_config.GetSourceTopic()) GenerateSourceTopicFeatures(sourceForms, phraseTopics, fc);
  if (m_config.GetSourceInternal()) GenerateInternalFeatures(sourceForms, fc);
  if(m_config.GetSourceIndicator()) GenerateIndicatorFeature(sourceForms, fc);
  if (m_config.GetBagOfWords()) GenerateBagOfWordsFeatures(context, spanStart, spanEnd, FACTOR_FORM, fc);
  if (m_config.GetSyntaxParent()) GenerateSyntaxFeatures(syntaxLabels,parent,span,fc);

  vector<ChartTranslation>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    fc->SetNamespace('t', false);

    // get words in target phrase
    //WRONG INDEX !!!
    vector<string> targetForms = Tokenize(m_targetIndex.right.find(transIt->m_index)->second, " ");

    if (m_config.GetTargetInternal()) GenerateInternalFeaturesChart(targetForms, fc, transIt->m_nonTermAlignment);

    if (m_config.GetPaired()) GeneratePairedFeaturesChart(sourceForms, targetForms, transIt->m_termAlignment, transIt->m_nonTermAlignment, fc);

    if (m_config.GetMostFrequent() && Equals(transIt->m_scores[P_E_F_INDEX], maxProb))
      fc->AddFeature("MOST_FREQUENT");

    if (m_config.GetBinnedScores()) GenerateScoreFeatures(transIt->m_scores, fc);

    if(m_config.GetTargetIndicator()) GenerateIndicatorFeatureChart(targetForms, fc, transIt->m_nonTermAlignment);

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
  m_scoreBins = Scan<float>(Tokenize(pTree.get<string>("features.score-bins", ""), ","));
  m_syntaxParent = pTree.get<bool>("features.syntax-parent", false);

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
  string parent = "[X]";
  string indicString = "";

  size_t sizeOfSpan = span.size();
  for (int i=0; i < sizeOfSpan; i++) {
    if( span[i].compare(parent) )
    {
        if (indicString.size()>0)
            indicString += "_";
        indicString += span[i];
    }
  }
  fc->AddFeature("p^" + indicString);
}

void FeatureExtractor::GenerateIndicatorFeatureChart(const vector<string> &span, FeatureConsumer *fc,AlignmentType nonTermAlign)
{
  string parent = "[X]";
  string indicString = "";
  string nonTerm = "[X][X]";
  size_t nonTermCounter = 0;

  size_t found;
  size_t sizeOfSpan = span.size();
  for (int i=0; i < sizeOfSpan; i++) {
    if( span[i].compare(parent) )
    {
        found = span[i].find(nonTerm);
        if(found != string::npos)
        {
            size_t newTerm = nonTermAlign.lower_bound(nonTermCounter)->second;
            ostringstream s1;
            s1 << newTerm;
            string sourceAlign =s1.str();

            if (indicString.size()>0)
             {indicString += "_";}
            indicString += nonTerm;
            indicString += sourceAlign;
            nonTermCounter++;
        }
        else{
        if (indicString.size()>0)
            {indicString += "_";}
        indicString += span[i];}
    }
  }
  fc->AddFeature("p^" + indicString);
}


void FeatureExtractor::GenerateInternalFeatures(const vector<string> &span, FeatureConsumer *fc)
{
  string parent = "[X]";
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    if(  (*it).compare(parent) )
    fc->AddFeature("w^" + *it);
  }
}

void FeatureExtractor::GenerateInternalFeaturesChart(const vector<string> &span, FeatureConsumer *fc,AlignmentType nonTermAlign)
{
  string parent = "[X]";
  string nonTerm = "[X][X]";
  size_t nonTermCounter = 0;

  size_t found;
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    size_t found = (*it).find(nonTerm);
    if(  (*it).compare(parent) )
    {
        if(found != string::npos)
        {
            size_t newTerm = nonTermAlign.lower_bound(nonTermCounter)->second;
            ostringstream s1;
            s1 << newTerm;
            string sourceAlign =s1.str();

            fc->AddFeature("w^" + nonTerm + sourceAlign);
            nonTermCounter++;
        }
        else{
        fc->AddFeature("w^" + *it);}
    }
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


void FeatureExtractor::GeneratePairedFeaturesChart(const vector<string> &srcPhrase, const vector<string> &tgtPhrase,
    const AlignmentType &alignTerm, const AlignmentType &alignNonTerm, FeatureConsumer *fc)
{
  /*vector<string> :: const_iterator itr_source;
  for(itr_source = srcPhrase.begin(); itr_source != srcPhrase.end(); itr_source++)
  {cerr << "Source : " << *itr_source << endl;}

   vector<string> :: const_iterator itr_target;
  for(itr_target = tgtPhrase.begin(); itr_target != tgtPhrase.end(); itr_target++)
  {cerr << "Target : " << *itr_target << endl;}*/

  AlignmentType::const_iterator it;
  set<size_t> srcAligned;
  set<size_t> tgtAligned;

  string parent = "[X]";
  string nonTerm = "[X][X]";

  for (it = alignTerm.begin(); it != alignTerm.end(); it++)
  {
    //cerr << "Alignment : " << it->first << " : " << it->second << endl;
    CHECK(it->first < srcPhrase.size());
    CHECK(it->second < tgtPhrase.size());
    if(srcPhrase[it->first].compare(nonTerm))
    {fc->AddFeature("tpair^" + srcPhrase[it->first] + "^" + tgtPhrase[it->second]);}
    srcAligned.insert(it->first);
    tgtAligned.insert(it->second);
      for (size_t i = 0; i < srcPhrase.size(); i++) {
        size_t found = srcPhrase[i].find(nonTerm);
        if (srcAligned.count(i) == 0 && srcPhrase[i].compare(parent) &&  !(found!=string::npos) )
        fc->AddFeature("tpair^" + srcPhrase[i] + "^NULL");
      }

      for (size_t i = 0; i < tgtPhrase.size(); i++) {
        size_t found = tgtPhrase[i].find(nonTerm);
        if (tgtAligned.count(i) == 0 && tgtPhrase[i].compare(parent) &&  !(found!=string::npos) )
          fc->AddFeature("tpair^NULL^" + tgtPhrase[i]);
      }
  }

  for (it = alignNonTerm.begin(); it != alignNonTerm.end(); it++)
  {
     ostringstream s1;
     s1 << it->first;
     string sourceAlign =s1.str();

     ostringstream s2;
     s2 << it->second;
     string targetAlign =s2.str();

    //cerr << "Alignment : " << it->first << " : " << it->second << endl;
    fc->AddFeature("ntpair^X" + sourceAlign + "^X" + targetAlign);
  }

}

void FeatureExtractor::GenerateScoreFeatures(const std::vector<float> scores, FeatureConsumer *fc)
{
  vector<size_t>::const_iterator scoreIt;
  vector<float>::const_iterator binIt;
  const vector<size_t> &scoreIDs = m_config.GetScoreIndexes();
  const vector<float> &bins = m_config.GetScoreBins();

  for (scoreIt = scoreIDs.begin(); scoreIt != scoreIDs.end(); scoreIt++) {
    for (binIt = bins.begin(); binIt != bins.end(); binIt++) {
      float logScore = log(scores[*scoreIt]);
      if (logScore < *binIt || Equals(logScore, *binIt))
        fc->AddFeature("sc^" + SPrint<size_t>(*scoreIt) + "_" + SPrint(*binIt));
    }
  }
}

//Labels
void FeatureExtractor::GenerateSyntaxFeatures(const std::vector<std::string> &syntaxLabel,
                                           const std::string parent, const std::string span,
                                           FeatureConsumer *fc)
{

  string noTag = "NOTAG";
  fc->AddFeature("span^" + span);


  //if several labels in vector repeats parent
  vector<string>::const_iterator it;
  for (it = syntaxLabel.begin(); it != syntaxLabel.end(); it++) {
     //cerr << "I am a syntax label : " << *it << endl;

     fc->AddFeature("con^" + *it);
     fc->AddFeature("con^" + *it + "^span^" + span);
     if( !(*it).compare(noTag) )
     {
         //cerr << "I am a notag, here is my parent : " << parent << endl;
         fc->AddFeature("inc^" + parent);
         fc->AddFeature("con^" + *it + "^inc^" + parent);
     }
     else
     {
         //cerr << "I am a const, here is my parent : " << parent << endl;
         fc->AddFeature("cmp");
         fc->AddFeature("cmp^span^" + span);
         fc->AddFeature("ins^" + parent);
         fc->AddFeature("cmp^ins^" + parent);
     }
  }
}

//CCG with parent only


} // namespace PSD

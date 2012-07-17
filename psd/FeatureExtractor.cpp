#include "FeatureExtractor.h"
#include "Util.h"

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

void FeatureExtractor::GenerateFeatures(FeatureConsumer *fc,
  const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  const vector<size_t> &translations,
  vector<float> &losses)
{
  fc->SetNamespace('s', true);
  if (m_config.GetSourceExternal()) {
    GenerateContextFeatures(context, spanStart, spanEnd, fc);
  }

  if (m_config.GetSourceInternal()) {
    vector<string> sourceForms(spanEnd - spanStart + 1);
    for (size_t i = spanStart; i <= spanEnd; i++) {
      sourceForms[i - spanStart] = context[i][0]; // XXX assumes that form is the 0th factor
    }
    GenerateInternalFeatures(sourceForms, fc);
  }

  vector<size_t>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    fc->SetNamespace('t', false);
    if (m_config.GetTargetInternal()) {
      GenerateInternalFeatures(Tokenize(m_targetIndex.right.find(*transIt)->second, " "), fc);
    }

    if (m_train) {
      fc->Train(SPrint(*transIt), *lossIt);
    } else {
      *lossIt = fc->Predict(SPrint(*transIt));
    }
  }
  fc->FinishExample();
}


void FeatureExtractor::GenerateFeaturesChart(FeatureConsumer *fc,
  const ContextType &context,
  const string &sourceSide,
  vector<string> &parentLabels,
  size_t spanStart,
  size_t spanEnd,
  const vector<size_t> &translations,
  vector<float> &losses)
{
  fc->SetNamespace('s', true);
  if (m_config.GetSourceExternal()) {
    GenerateContextFeatures(context, spanStart, spanEnd, fc);
  }

  if (m_config.GetSourceInternal()) {
    vector<string> sourceToken = Tokenize(sourceSide, " ");
    GenerateInternalFeatures(sourceToken, fc);
  }

    cerr << "Syntax parent : " << m_config.GetSyntaxParent() << endl;
   if (m_config.GetSyntaxParent()) {
    GenerateSyntaxFeatures(parentLabels,fc);
  }

  vector<size_t>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    fc->SetNamespace('t', false);
    if (m_config.GetTargetInternal()) {
      GenerateInternalFeatures(Tokenize(m_targetIndex.right.find(*transIt)->second, " "), fc);
    }
    if (m_train) {
      fc->Train(SPrint(*transIt), *lossIt);
    } else {
      *lossIt = fc->Predict(SPrint(*transIt));
    }
  }
  fc->FinishExample();
}

ExtractorConfig::ExtractorConfig()

  : m_paired(false), m_bagOfWords(false), m_sourceExternal(false),
         m_sourceInternal(false), m_targetInternal(false), m_syntaxParent(false), m_windowSize(0)
{}

void ExtractorConfig::Load(const string &configFile)
{
  ptree pTree;

  std::cerr << "CONFIG FILE : " << configFile << std::endl;

  ini_parser::read_ini(configFile, pTree);
  m_sourceInternal = pTree.get<bool>("features.source-internal", false);
  m_sourceExternal = pTree.get<bool>("features.source-external", false);
  m_targetInternal = pTree.get<bool>("features.target-internal", false);
  m_syntaxParent = pTree.get<bool>("features.syntax-parent", false);
  m_paired         = pTree.get<bool>("features.paired", false);
  m_bagOfWords     = pTree.get<bool>("features.bag-of-words", false);
  m_windowSize     = pTree.get<size_t>("features.window-size", 0);

  vector<string> factors = Tokenize(pTree.get<string>("features.factors", ""),",");
  vector<string>::const_iterator it;
  for (it = factors.begin(); it != factors.end(); it++) {
    m_factors.push_back(Scan<size_t>(*it));
  }
  m_isLoaded = true;
}

//
// private methods
//

string FeatureExtractor::BuildContextFeature(size_t factor, int index, const string &value)
{
  string featureString = "c^" + SPrint(factor) + "_-" + SPrint(index) + "_" + value;
  std::cout << "Adding source context feature..." << featureString <<  std::endl;
  return featureString;
}

void FeatureExtractor::GenerateContextFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  FeatureConsumer *fc)
{
  for (size_t fact = 0; fact < m_config.GetFactors().size(); fact++) {
      cerr << "Factor : " << fact << endl;
    for (size_t i = 1; i <= m_config.GetWindowSize(); i++) {
      cerr << "Start : " << spanStart << " : " << i << endl;
      if (spanStart >= i)
        //TODO : GET FACTORS FROM CONFIG
        fc->AddFeature(BuildContextFeature(fact, i, context[spanStart - i][fact]));
      //cerr << "End : " << spanEnd << " : " << i << endl;
      if (spanEnd + i < context.size())
        fc->AddFeature(BuildContextFeature(fact, i, context[spanStart + i][fact]));
    }
  }
}

void FeatureExtractor::GenerateInternalFeatures(const vector<string> &span, FeatureConsumer *fc)
{
  fc->AddFeature("p^" + Join("_", span));
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    fc->AddFeature("w^" + *it);
  }
  std::cout << "Adding internal feature..." << Join( "_",span) <<  std::endl;
}

void FeatureExtractor::GenerateSyntaxFeatures(const std::vector<std::string> &syntaxLabel, FeatureConsumer *fc)
{
  vector<string>::const_iterator it;
  for (it = syntaxLabel.begin(); it != syntaxLabel.end(); it++) {
     fc->AddFeature("sp^" + *it);
  }
}



}//namespace

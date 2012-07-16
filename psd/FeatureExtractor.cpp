#include "FeatureExtractor.h"
#include "Util.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <exception>
#include <stdexcept>

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

ExtractorConfig::ExtractorConfig()

  : m_paired(false), m_bagOfWords(false), m_sourceExternal(false),
         m_sourceInternal(false), m_targetInternal(false), m_windowSize(0)
{}

void ExtractorConfig::Load(const string &configFile)
{
  ptree pTree;
  ini_parser::read_ini(configFile, pTree);
  m_sourceInternal = pTree.get<bool>("features.source-internal", false);
  m_sourceExternal = pTree.get<bool>("features.source-external", false);
  m_targetInternal = pTree.get<bool>("features.target-internal", false);
  m_paired         = pTree.get<bool>("features.paired", false);
  m_bagOfWords     = pTree.get<bool>("features.bag-of-words", false);
  m_windowSize     = pTree.get<size_t>("features.window-size", 0);  

  vector<string> factors = Tokenize(",", pTree.get<string>("features.factors", ""));
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
  return "c^" + SPrint(factor) + "_" + SPrint(index) + "_" + value;
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

void FeatureExtractor::GenerateInternalFeatures(const vector<string> &span, FeatureConsumer *fc)
{
  fc->AddFeature("p^" + Join("_", span));
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    fc->AddFeature("w^" + *it);
  }
}

} // namespace PSD

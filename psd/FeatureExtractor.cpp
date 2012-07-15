#include "FeatureExtractor.h"
#include "Util.h"
#include "StaticData.h"

using namespace std;
using namespace boost::bimaps;
using namespace Moses;

namespace PSD
{

FeatureExtractor::FeatureExtractor(const TargetIndexType &targetIndex,
  bool train)
  : m_targetIndex(targetIndex), m_train(train){}

void FeatureExtractor::GenerateFeatures(FeatureConsumer *fc,
  const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  const vector<size_t> &translations,
  vector<float> &losses)
{
  fc->SetNamespace('s', true);
  if (PSD_SOURCE_EXTERNAL) {
    GenerateContextFeatures(context, spanStart, spanEnd, fc);
  }

  if (PSD_SOURCE_INTERNAL) {
    vector<string> sourceForms(spanEnd - spanStart + 1);
    for (size_t i = spanStart; i <= spanEnd; i++) {
      sourceForms[i] = context[i][0]; // XXX assumes that form is the 0th factor
    }
    GenerateInternalFeatures(sourceForms, fc);
  }

  vector<size_t>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    fc->SetNamespace('t', false);
    if (PSD_TARGET_INTERNAL) {
      GenerateInternalFeatures(Tokenize(" ", m_targetIndex.right.find(*transIt)->second), fc);
    }

    if (m_train) {
      fc->Train(SPrint(*transIt), *lossIt);
    } else {
      *lossIt = fc->Predict(SPrint(*transIt));
    }
  }
}

void FeatureExtractor::GenerateFeaturesChart(FeatureConsumer *fc,
  const ContextType &context,
  vector<string> sourceSide,
  size_t spanStart,
  size_t spanEnd,
  const vector<size_t> &translations,
  vector<float> &losses)
{
  fc->SetNamespace('s', true);
  if (PSD_SOURCE_EXTERNAL) {
    GenerateContextFeatures(context, spanStart, spanEnd, fc);
  }

  if (PSD_SOURCE_INTERNAL) {
    GenerateInternalFeatures(sourceSide, fc);
  }

   if (SYNTAX_PARENT) {
      std::cout << "Generate syntactic parent feature" << std::endl;
  }

  vector<size_t>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    fc->SetNamespace('t', false);
    if (PSD_TARGET_INTERNAL) {
      GenerateInternalFeatures(Tokenize(" ", m_targetIndex.right.find(*transIt)->second), fc);
    }

    if (m_train) {
      fc->Train(SPrint(*transIt), *lossIt);
    } else {
      *lossIt = fc->Predict(SPrint(*transIt));
    }
  }
}

//
// private methods
//

string FeatureExtractor::BuildContextFeature(size_t factor, int index, const string &value)
{
  string featureString = "c^" + SPrint(factor) + "_-" + SPrint(index) + "_" + value;
  //Moses::VERBOSE(5, "Adding source context feature..." << featureString <<  endl);
  return featureString;
}

void FeatureExtractor::GenerateContextFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  FeatureConsumer *fc)
{
  for (size_t fact = 0; fact <= PSD_FACTOR_COUNT; fact++) {
    for (size_t i = 1; i <= PSD_CONTEXT_WINDOW; i++) {
      if (spanStart >= i)
        fc->AddFeature(BuildContextFeature(fact, i, context[spanStart - i][fact]));
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
}

}//namespace

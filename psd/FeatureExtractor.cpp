#include "FeatureExtractor.h"
#include "Util.h"

using namespace std;
using namespace boost::bimaps;
using namespace Moses;

FeatureExtractor::FeatureExtractor(FeatureTypes ft,
  const TargetIndexType &targetIndex,
  bool train)
  : m_ft(ft), m_targetIndex(targetIndex), m_train(train)
{  
}

void FeatureExtractor::GenerateFeatures(FeatureConsumer *fc,
  const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  const vector<size_t> &translations,
  vector<float> &losses)
{  
  fc->SetNamespace('s', true);
  if (m_ft.m_sourceExternal) {
    GenerateContextFeatures(context, spanStart, spanEnd, fc);
  }

  if (m_ft.m_sourceInternal) {
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
    if (m_ft.m_targetInternal) {
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
  return "c^" + SPrint(factor) + "_-" + SPrint(index) + "_" + value;
}

void FeatureExtractor::GenerateContextFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  FeatureConsumer *fc)
{
  vector<size_t>::const_iterator factorIt;
  for (factorIt = m_ft.m_factors.begin(); factorIt != m_ft.m_factors.end(); factorIt++) {
    for (size_t i = 1; i <= m_ft.m_contextWindow; i++) {
      if (spanStart >= i) 
        fc->AddFeature(BuildContextFeature(*factorIt, i, context[spanstart - i][*factorit]);
      if (spanEnd + i < context.size())
        fc->AddFeature(BuildContextFeature(*factorIt, i, context[spanstart - i][*factorit]);
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


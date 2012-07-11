#include "FeatureExtractor.h"
#include "Util.h"

using namespace std;
using namespace boost::bimaps;
using namespace Moses;

FeatureExtractor::FeatureExtractor(FeatureTypes ft,
  FeatureConsumer *fc,
  const TargetIndexType &targetIndex,
  bool train)
  : m_ft(ft), m_fc(fc), m_targetIndex(targetIndex), m_train(train)
{  
}

void FeatureExtractor::GenerateContextFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd)
{
  vector<size_t>::const_iterator factorIt;
  for (factorIt = m_ft.m_factors.begin(); factorIt != m_ft.m_factors.end(); factorIt++) {
    for (size_t i = 1; i <= m_ft.m_contextWindow; i++) {
      if (spanStart >= i) {
        m_fc->AddFeature("c^-" + SPrint(i) + "_" + context[spanStart - i][*factorIt]);
      }    
      if (spanEnd + i < context.size()) {
        m_fc->AddFeature("c^" + SPrint(i) + "_" + context[spanStart + i][*factorIt]);
      }
    }
  }
}

void FeatureExtractor::GenerateInternalFeatures(const vector<string> &span)
{
  m_fc->AddFeature("p^" + Join(" ", span));
  vector<string>::const_iterator it;
  for (it = span.begin(); it != span.end(); it++) {
    m_fc->AddFeature("w^" + *it);
  }
}

void FeatureExtractor::GenerateFeatures(const ContextType &context,
  size_t spanStart,
  size_t spanEnd,
  const vector<size_t> &translations,
  vector<float> &losses)
{  
  m_fc->SetNamespace('s', true);
  if (m_ft.m_sourceExternal) {
    GenerateContextFeatures(context, spanStart, spanEnd);
  }

  if (m_ft.m_sourceInternal) {
    vector<string> sourceForms(spanEnd - spanStart + 1);
    for (size_t i = spanStart; i <= spanEnd; i++) {
      sourceForms[i] = context[i][0]; // XXX assumes that form is the 0th factor
    }
    GenerateInternalFeatures(sourceForms);
  }

  vector<size_t>::const_iterator transIt = translations.begin();
  vector<float>::iterator lossIt = losses.begin();
  for (; transIt != translations.end(); transIt++, lossIt++) {
    assert(lossIt != losses.end());
    m_fc->SetNamespace('t', false);
    if (m_ft.m_targetInternal) {
      GenerateInternalFeatures(Tokenize(" ", m_targetIndex.right.find(*transIt)->second));
    }  

    if (m_train) {
      m_fc->Train(SPrint(*transIt), *lossIt);
    } else {
      *lossIt = m_fc->Predict(SPrint(*transIt));
    }
  }
}

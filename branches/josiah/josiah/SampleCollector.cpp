#include "SampleCollector.h"
#include "Gibbler.h"
#include "GibbsOperator.h"

namespace Josiah {
  
  void PrintSampleCollector::collect(Sample& sample)  {
    cout << "Sampled hypothesis: \"";
    sample.GetSampleHypothesis()->ToStream(cout);
    cout << "\"" << "  " << "Feature values: " << sample.GetFeatureValues() << endl;
  }
  
  void SampleCollector::addSample( Sample & sample, double importanceWeight ) {
    if (m_importanceWeights.empty()) {
      m_totalImportanceWeight = importanceWeight;
    } else {
      m_totalImportanceWeight = log_sum(m_totalImportanceWeight,importanceWeight);
    }
    m_importanceWeights.push_back(importanceWeight);
    collect(sample);
    ++m_n;
  }
  
  const vector<double>& SampleCollector::getImportanceWeights() const {
    size_t impSize = m_importanceWeights.size();
    if (m_normalisedImportanceWeights.size() != impSize) {
      m_normalisedImportanceWeights.resize(impSize,0);
      for (size_t i = 0; i < impSize; ++i) {
        double logNormalisedWeight = max(-100.0, m_importanceWeights[i] - m_totalImportanceWeight);
        m_normalisedImportanceWeights[i] = exp(logNormalisedWeight);
      }
    }
    return m_normalisedImportanceWeights;
  }
}

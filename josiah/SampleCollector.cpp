#include "SampleCollector.h"
#include "Gibbler.h"
#include "GibbsOperator.h"

using namespace std;

namespace Josiah {
  
  void PrintSampleCollector::collect(Sample& sample)  {
    cout << "Sampled hypothesis: \"";
    sample.GetSampleHypothesis()->ToStream(cout);
    cout << "\"" << "  " << "Feature values: " << sample.GetFeatureValues() << endl;
  }
  
  void SampleCollector::addSample( Sample & sample) {
    collect(sample);
    ++m_n;
  }
}

#include "GibblerExpectedLossTraining.h"

#include "GainFunction.h"
#include "Hypothesis.h"

using namespace std;

namespace Josiah {

void GibblerExpectedLossCollector::collect(Sample& s) {
  ++n;  // increment total samples seen
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const float gain = g.ComputeGain(trans);
  tot_len += trans.size();
  VERBOSE(2, gain << "\tFeatures=" << s.GetFeatureValues() << endl);
  samples.push_back(make_pair(s.GetFeatureValues(), gain));
  feature_expectations.PlusEquals(s.GetFeatureValues());
}

float GibblerExpectedLossCollector::UpdateGradient(ScoreComponentCollection* gradient,
    float *exp_len) {
  feature_expectations.DivideEquals(n);
  ScoreComponentCollection grad = feature_expectations; grad.ZeroAll();
  list<pair<ScoreComponentCollection, float> >::iterator si;
  double exp_gain = 0;
  for (si = samples.begin(); si != samples.end(); ++si) {
    ScoreComponentCollection d = si->first;
    const float gain = si->second;
    d.MinusEquals(feature_expectations);
    d.MultiplyEquals(gain);
    exp_gain += gain;
    grad.PlusEquals(d);
  }
  exp_gain /= static_cast<double>(n);
  grad.DivideEquals(n);

  cerr << "Gradient: " << grad << endl;
  cerr << "Exp gain:  " << exp_gain << endl;
  if (exp_len)
    *exp_len = static_cast<float>(tot_len) / n;


  gradient->PlusEquals(grad);
  return exp_gain;
}
}


#include "GibblerExpectedLossTraining.h"

using namespace std;

namespace Moses {

GibblerExpectedLossCollector::GibblerExpectedLossCollector() :
  g(NULL), sent_num(0) {}

void GibblerExpectedLossCollector::collect(Sample& s) {
  ++n;  // increment total samples seen
  const Hypothesis* h = s.GetSampleHypothesis();
  const float gain = g->ComputeGain(h, refs[sent_num]);
  samples.push_back(make_pair(h->GetScoreBreakdown(), gain));
  feature_expectations.PlusEquals(h->GetScoreBreakdown());
}

ScoreComponentCollection GibblerExpectedLossCollector::ComputeGradient() {
  feature_expectations.DivideEquals(n);
  ScoreComponentCollection grad = feature_expectations; grad.ZeroAll();
  list<pair<ScoreComponentCollection, float> >::iterator si;
  for (si = samples.begin(); si != samples.end(); ++si) {
    ScoreComponentCollection d = si->first;
    const float gain = si->second;
    d.MinusEquals(feature_expectations);
    d.MultiplyEquals(gain);
    grad.PlusEquals(d);
  }
  grad.DivideEquals(n);

  gradient.PlusEquals(grad);
}

}


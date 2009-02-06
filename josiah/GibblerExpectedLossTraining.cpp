#include "GibblerExpectedLossTraining.h"

#include "Phrase.h"

using namespace std;

namespace Josiah {

void GainFunction::ConvertStringToFactorArray(const std::string& str, std::vector<const Factor*>* out) {
  Phrase phrase(Output);
  vector<FactorType> ft(1, 0);
  phrase.CreateFromString(ft, str, "|");
  out->resize(phrase.GetSize());
  FactorType type = ft.front();
  for (unsigned i = 0; i < phrase.GetSize(); ++i)
    (*out)[i] = phrase.GetFactor(i, type);
}

GainFunction::~GainFunction() {}

GibblerExpectedLossCollector::GibblerExpectedLossCollector() :
  g(NULL), sent_num(0) {}

void GibblerExpectedLossCollector::collect(Sample& s) {
  ++n;  // increment total samples seen
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const float gain = g->ComputeGain(trans);
  samples.push_back(make_pair(s.GetFeatureValues(), gain));
  feature_expectations.PlusEquals(s.GetFeatureValues());
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


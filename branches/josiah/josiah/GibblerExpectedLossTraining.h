#pragma once

#include <utility>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

using namespace Moses;

namespace Josiah {

class GainFunction {
 public:
  virtual ~GainFunction();
  virtual float ComputeGain(const std::vector<const Factor*>& hyp) const = 0;

  static void ConvertStringToFactorArray(const std::string& str, std::vector<const Factor*>* out);
};

class GibblerExpectedLossCollector : public SampleCollector {
 public:
  GibblerExpectedLossCollector();
  virtual void collect(Sample& sample);
  void SetCurrentSentenceNumber(int n) { sent_num = n; }
  ScoreComponentCollection ComputeGradient();

 private:
  int n;
  ScoreComponentCollection gradient;
  std::list<std::pair<ScoreComponentCollection, float> > samples;
  ScoreComponentCollection feature_expectations;

  const GainFunction* g;
  int sent_num;
  std::vector<std::vector<std::vector<const Factor*> > > refs;
};

}

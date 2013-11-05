#include <cassert>
#include "StatefulFeatureFunction.h"
#include "Search/Hypothesis.h"

std::vector<StatefulFeatureFunction*> StatefulFeatureFunction::s_staticColl;

StatefulFeatureFunction::StatefulFeatureFunction(const std::string line)
  :FeatureFunction(line)
{
  s_staticColl.push_back(this);
}

StatefulFeatureFunction::~StatefulFeatureFunction()
{
  // TODO Auto-generated destructor stub
}

///////////////////////////////////////////////////

void StatefulFeatureFunction::Evaluate(Hypothesis& hypo)
{
  const Hypothesis &prevHypo = *hypo.GetPrevHypo();
  Scores &scores = hypo.GetScores();
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    const StatefulFeatureFunction &ff = *s_staticColl[i];
    size_t prevFFState = prevHypo.GetState(i);

    size_t ffState = ff.Evaluate(hypo, prevFFState, scores);
    assert(ffState);
    hypo.SetState(i, ffState);
  }  
}

void StatefulFeatureFunction::EvaluateEmptyHypo(const Sentence &input, Hypothesis& hypo)
{
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    const StatefulFeatureFunction &ff = *s_staticColl[i];
    size_t ffState = ff.EmptyHypo(input, hypo);
    hypo.SetState(i, ffState);
  }
}


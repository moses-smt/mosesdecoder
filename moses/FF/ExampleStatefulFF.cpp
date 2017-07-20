#include <vector>
#include "ExampleStatefulFF.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"

using namespace std;

namespace Moses
{

////////////////////////////////////////////////////////////////
ExampleStatefulFF::ExampleStatefulFF(const std::string &line)
  :StatefulFeatureFunction(3, line)
{
  ReadParameters();
}


// An empty implementation of this function is provided by StatefulFeatureFunction.
// Unless you are actually implementing this, please remove it from your
// implementation (and the declaration in the header file to reduce code clutter.
void ExampleStatefulFF::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedScores) const
{}

// An empty implementation of this function is provided by StatefulFeatureFunction.
// Unless you are actually implementing this, please remove it from your
// implementation (and the declaration in the header file to reduce code clutter.
void ExampleStatefulFF::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{}

// An empty implementation of this function is provided by StatefulFeatureFunction.
// Unless you are actually implementing this, please remove it from your
// implementation (and the declaration in the header file to reduce code clutter.
void ExampleStatefulFF::EvaluateTranslationOptionListWithSourceContext
(const InputType &input, const TranslationOptionList &translationOptionList) const
{}

FFState* ExampleStatefulFF::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  newScores[0] = 1.5;
  newScores[1] = 0.3;
  newScores[2] = 0.4;
  accumulator->PlusEquals(this, newScores);

  // sparse scores
  accumulator->PlusEquals(this, "sparse-name", 2.4);

  // int targetLen = cur_hypo.GetCurrTargetPhrase().GetSize(); // ??? [UG]
  return new ExampleState(0);
}

FFState* ExampleStatefulFF::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new ExampleState(0);
}

void ExampleStatefulFF::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "arg") {
    // set value here
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}


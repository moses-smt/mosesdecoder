#include <vector>
#include "ExampleStatelessFF.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"

using namespace std;

namespace Moses
{
ExampleStatelessFF::ExampleStatelessFF(const std::string &line)
  :StatelessFeatureFunction(2, line)
{
  ReadParameters();
}

void ExampleStatelessFF::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedScores) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  newScores[0] = 1.5;
  newScores[1] = 0.3;
  scoreBreakdown.PlusEquals(this, newScores);

  // sparse scores
  scoreBreakdown.PlusEquals(this, "sparse-name", 2.4);

}

void ExampleStatelessFF::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{
  if (targetPhrase.GetNumNonTerminals()) {
    vector<float> newScores(m_numScoreComponents);
    newScores[0] = - std::numeric_limits<float>::infinity();
    scoreBreakdown.PlusEquals(this, newScores);
  }
}

void ExampleStatelessFF::EvaluateTranslationOptionListWithSourceContext(const InputType &input

    , const TranslationOptionList &translationOptionList) const
{}

void ExampleStatelessFF::EvaluateWhenApplied(const Hypothesis& hypo,
    ScoreComponentCollection* accumulator) const
{}

void ExampleStatelessFF::EvaluateWhenApplied(const ChartHypothesis &hypo,
    ScoreComponentCollection* accumulator) const
{}

void ExampleStatelessFF::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "arg") {
    // set value here
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}


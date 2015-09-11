#include <vector>
#include <string>
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/FF/NeuralScoreFeature.h"
#include "moses/FF/NMT/NMT_Wrapper.h"

using namespace std;

namespace Moses
{
NeuralScoreFeature::NeuralScoreFeature(const std::string &line)
  :StatelessFeatureFunction(1, line)
{
  ReadParameters();
  NMT_Wrapper* wrapper = new NMT_Wrapper();
  wrapper->Init(statePath, modelPath, wrapperPath);
}

void NeuralScoreFeature::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  newScores[0] = 1.5;
  scoreBreakdown.PlusEquals(this, newScores);

  // sparse scores
  scoreBreakdown.PlusEquals(this, "sparse-name", 2.4);

}

void NeuralScoreFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{
  if (targetPhrase.GetNumNonTerminals()) {
    vector<float> newScores(m_numScoreComponents);
    newScores[0] = - std::numeric_limits<float>::infinity();
    scoreBreakdown.PlusEquals(this, newScores);
  }
}

void NeuralScoreFeature::EvaluateTranslationOptionListWithSourceContext(const InputType &input

    , const TranslationOptionList &translationOptionList) const
{}

void NeuralScoreFeature::EvaluateWhenApplied(const Hypothesis& hypo,
    ScoreComponentCollection* accumulator) const
{}

void NeuralScoreFeature::EvaluateWhenApplied(const ChartHypothesis &hypo,
    ScoreComponentCollection* accumulator) const
{}

void NeuralScoreFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "state") {
    statePath = value;
  } else if (key == "model") {
      modelPath = value;
  } else if (key == "wrapper-path") {
      wrapperPath = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}


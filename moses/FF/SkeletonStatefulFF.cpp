#include <vector>
#include "SkeletonStatefulFF.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"

using namespace std;

namespace Moses
{
int SkeletonState::Compare(const FFState& other) const
{
  const SkeletonState &otherState = static_cast<const SkeletonState&>(other);

  if (m_targetLen == otherState.m_targetLen)
    return 0;
  return (m_targetLen < otherState.m_targetLen) ? -1 : +1;
}

////////////////////////////////////////////////////////////////
SkeletonStatefulFF::SkeletonStatefulFF(const std::string &line)
  :StatefulFeatureFunction(3, line)
{
  ReadParameters();
}

void SkeletonStatefulFF::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{}

void SkeletonStatefulFF::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{}

void SkeletonStatefulFF::EvaluateTranslationOptionListWithSourceContext(const InputType &input
    , const TranslationOptionList &translationOptionList) const
{}

FFState* SkeletonStatefulFF::EvaluateWhenApplied(
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
  return new SkeletonState(0);
}

FFState* SkeletonStatefulFF::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new SkeletonState(0);
}

void SkeletonStatefulFF::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "arg") {
    // set value here
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}


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

void SkeletonStatefulFF::Evaluate(const Phrase &source
                                  , const TargetPhrase &targetPhrase
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection &estimatedFutureScore) const
{}

void SkeletonStatefulFF::Evaluate(const InputType &input
                                  , const InputPath &inputPath
                                  , const TargetPhrase &targetPhrase
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection *estimatedFutureScore) const
{}

FFState* SkeletonStatefulFF::Evaluate(
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

  int targetLen = cur_hypo.GetCurrTargetPhrase().GetSize();
  return new SkeletonState(0);
}

FFState* SkeletonStatefulFF::EvaluateChart(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new SkeletonState(0);
}


}


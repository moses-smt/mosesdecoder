#include "SkeletonStatelessFF.h"
#include "moses/ScoreComponentCollection.h"
#include <vector>

using namespace std;

namespace Moses
{
void SkeletonStatelessFF::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  newScores[0] = 1.5;
  newScores[1] = 0.3;
  scoreBreakdown.PlusEquals(this, newScores);

  // sparse scores
  scoreBreakdown.PlusEquals(this, "sparse-name", 2.4);

}

void SkeletonStatelessFF::Evaluate(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{}

void SkeletonStatelessFF::Evaluate(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void SkeletonStatelessFF::EvaluateChart(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

}


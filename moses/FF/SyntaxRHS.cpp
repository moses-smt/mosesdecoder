#include <vector>
#include "SyntaxRHS.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/StackVec.h"

using namespace std;

namespace Moses
{
SyntaxRHS::SyntaxRHS(const std::string &line)
  :StatelessFeatureFunction(1, line)
{
  ReadParameters();
}

void SyntaxRHS::EvaluateInIsolation(const Phrase &source
                                    , const TargetPhrase &targetPhrase
                                    , ScoreComponentCollection &scoreBreakdown
                                    , ScoreComponentCollection &estimatedFutureScore) const
{
}

void SyntaxRHS::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{
  assert(stackVec);
  for (size_t i = 0; i < stackVec->size(); ++i) {
    const ChartCellLabel &cell = *stackVec->at(i);

  }

  if (targetPhrase.GetNumNonTerminals()) {
    vector<float> newScores(m_numScoreComponents);
    newScores[0] = - std::numeric_limits<float>::infinity();
    scoreBreakdown.PlusEquals(this, newScores);
  }

}

}


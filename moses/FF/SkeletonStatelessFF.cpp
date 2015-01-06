#include <vector>
#include "SkeletonStatelessFF.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"

using namespace std;

namespace Moses
{
SkeletonStatelessFF::SkeletonStatelessFF(const std::string &line)
  :StatelessFeatureFunction(2, line)
{
  ReadParameters();
}

void SkeletonStatelessFF::EvaluateInIsolation(const Phrase &source
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

void SkeletonStatelessFF::EvaluateWithSourceContext(const InputType &input
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

void SkeletonStatelessFF::EvaluateTranslationOptionListWithSourceContext(const InputType &input

                , const TranslationOptionList &translationOptionList) const
{}

void SkeletonStatelessFF::EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void SkeletonStatelessFF::EvaluateWhenApplied(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

void SkeletonStatelessFF::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "arg") {
    // set value here
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}


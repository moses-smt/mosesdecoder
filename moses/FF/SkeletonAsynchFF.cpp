#include <vector>
#include "SkeletonAsynchFF.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"

using namespace std;

namespace Moses
{
SkeletonAsynchFF::SkeletonAsynchFF(const std::string &line)
  :AsynchFeatureFunction(2, line)
{
  ReadParameters();
}


void SkeletonAsynchFF::EvaluateNbest(const InputType &input,
		                     const TrellisPathList &Nbest) const 
{}

void SkeletonAsynchFF::EvaluateSearchGraph(const InputType &input,
	       				   const std::vector < HypothesisStack* > &hypoStackColl) const
{}

void SkeletonAsynchFF::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  newScores[0] = 0.0;
  newScores[1] = 0.0;
  scoreBreakdown.PlusEquals(this, newScores);

  // sparse scores
//   scoreBreakdown.PlusEquals(this, "sparse-name", );

}

void SkeletonAsynchFF::EvaluateWithSourceContext(const InputType &input
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

void SkeletonAsynchFF::EvaluateTranslationOptionListWithSourceContext(const InputType &input

    , const TranslationOptionList &translationOptionList) const
{}

void SkeletonAsynchFF::EvaluateWhenApplied(const Hypothesis& hypo,
    ScoreComponentCollection* accumulator) const
{}

void SkeletonAsynchFF::EvaluateWhenApplied(const ChartHypothesis &hypo,
    ScoreComponentCollection* accumulator) const
{}

void SkeletonAsynchFF::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "arg") {
    // set value here
  } else {
    AsynchFeatureFunction::SetParameter(key, value);
  }
}

}


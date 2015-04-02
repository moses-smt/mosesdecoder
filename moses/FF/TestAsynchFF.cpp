#include <vector>
#include "TestAsynchFF.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/Phrase.h"

using namespace std;

namespace Moses
{
TestAsynchFF::TestAsynchFF(const std::string &line)
  :AsynchFeatureFunction(1, line)
{
  ReadParameters();
}



void TestAsynchFF::EvaluateNbest(const InputType &input, const TrellisPathList &Nbest) const
{

	cout<<"Evaluate Nbest - Need size and CSLM file "<<endl;
}


void TestAsynchFF::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  // dense scores
  float newScore = 0.0 ; // nbWords = source.GetSize() + targetPhrase.GetSize();
//  vector<float> newScores(m_numScoreComponents);
//  newScores[0] = 1.5;
//  newScores[1] = 0.3;
//  cout<<"Evaluate TestAsynchFF "<< nbWords <<endl;
  scoreBreakdown.PlusEquals(this, newScore);//newScores);

}


void TestAsynchFF::EvaluateWithSourceContext(const InputType &input
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



void TestAsynchFF::EvaluateTranslationOptionListWithSourceContext(const InputType &input

    , const TranslationOptionList &translationOptionList) const
{}

/*
void TestAsynchFF::EvaluateWhenApplied(const Hypothesis& hypo,
    ScoreComponentCollection* accumulator) const
{}

void TestAsynchFF::EvaluateWhenApplied(const ChartHypothesis &hypo,
    ScoreComponentCollection* accumulator) const
{}

*/

void TestAsynchFF::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "arg") {
    // set value here
  } else {
    AsynchFeatureFunction::SetParameter(key, value);
  }
}

}


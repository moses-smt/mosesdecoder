#include <vector>
#include "SkeletonChangeInput.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/Sentence.h"
#include "moses/FactorCollection.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
SkeletonChangeInput::SkeletonChangeInput(const std::string &line)
  :StatelessFeatureFunction(2, line)
{
  ReadParameters();
}

void SkeletonChangeInput::EvaluateInIsolation(const Phrase &source
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

void SkeletonChangeInput::EvaluateWithSourceContext(const InputType &input
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

void SkeletonChangeInput::EvaluateTranslationOptionListWithSourceContext(const InputType &input
								  , const TranslationOptionList &translationOptionList) const
{}

void SkeletonChangeInput::EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void SkeletonChangeInput::EvaluateWhenApplied(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

void SkeletonChangeInput::ChangeSource(InputType *&input) const
{
  // add factor[1] to each word. Created from first 4 letter of factor[0]

  Sentence *sentence = dynamic_cast<Sentence*>(input);
  UTIL_THROW_IF2(sentence == NULL, "Not a sentence input");

  FactorCollection &fc = FactorCollection::Instance();

  size_t size = sentence->GetSize();
  for (size_t i = 0; i < size; ++i) {
	  Word &word = sentence->Phrase::GetWord(i);
	  const Factor *factor0 = word[0];

	  std::string str = factor0->GetString().as_string();
	  if (str.length() > 4) {
  	    str = str.substr(0, 4);
	  }

	  const Factor *factor1 = fc.AddFactor(str);
	  word.SetFactor(1, factor1);
  }
}

void SkeletonChangeInput::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "arg") {
    // set value here
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}


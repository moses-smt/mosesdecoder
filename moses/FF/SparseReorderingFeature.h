#pragma once

#include <string>

#include "StatefulFeatureFunction.h"
#include "FFState.h"

namespace Moses
{

class SparseReorderingState : public FFState
{
public:
	int Compare(const FFState& other) const
	{
		return 0;
	}
};

class SparseReorderingFeature : public StatefulFeatureFunction
{
public:
	SparseReorderingFeature(const std::string &line);

	bool IsUseable(const FactorMask &mask) const
		{ return true; }

	void Evaluate(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const
	{}
	void Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ScoreComponentCollection &scoreBreakdown) const
	{}
	  FFState* Evaluate(
	    const Hypothesis& cur_hypo,
	    const FFState* prev_state,
	    ScoreComponentCollection* accumulator) const
	  {
		  return new SparseReorderingState();
	  }

	  FFState* EvaluateChart(
	    const ChartHypothesis& /* cur_hypo */,
	    int /* featureID - used to index the state in the previous hypotheses */,
	    ScoreComponentCollection* accumulator) const;

	  virtual const FFState* EmptyHypothesisState(const InputType &input) const
	  {
		  return new SparseReorderingState();
	  }

private:

void AddNonTerminalPairFeatures(
  const Sentence& sentence, const WordsRange& nt1, const WordsRange& nt2,
    bool isMonotone, ScoreComponentCollection* accumulator) const;

};


}


#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Phrase.h"

namespace Moses
{

class ConstrainedDecodingState : public FFState
{
public:
	ConstrainedDecodingState()
	{}

	ConstrainedDecodingState(const ChartHypothesis &hypo);

	int Compare(const FFState& other) const;

	const Phrase &GetPhrase() const
	{ return m_outputPhrase; }

protected:
	Phrase m_outputPhrase;
};

class ConstrainedDecoding : public StatefulFeatureFunction
{
public:
	ConstrainedDecoding(const std::string &line)
		:StatefulFeatureFunction("ConstrainedDecoding", line)
		{}

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
	    ScoreComponentCollection* accumulator) const;

	  FFState* EvaluateChart(
	    const ChartHypothesis& /* cur_hypo */,
	    int /* featureID - used to index the state in the previous hypotheses */,
	    ScoreComponentCollection* accumulator) const;

	  virtual const FFState* EmptyHypothesisState(const InputType &input) const
	  {
		  return new ConstrainedDecodingState();
	  }


};


}


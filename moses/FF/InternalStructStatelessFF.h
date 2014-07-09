#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{

class InternalStructStatelessFF : public StatelessFeatureFunction
{
public:
	InternalStructStatelessFF(const std::string &line)
	:StatelessFeatureFunction(line)
	{}

	bool IsUseable(const FactorMask &mask) const
	{ return true; }

	void EvaluateInIsolation(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const;

	void EvaluateWithSourceContext(const InputType &input
	                        , const InputPath &inputPath
	                        , const TargetPhrase &targetPhrase
	                        , const StackVec *stackVec
	                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection *estimatedFutureScore = NULL) const;
	  virtual void EvaluateWhenApplied(const Hypothesis& hypo,
	                        ScoreComponentCollection* accumulator) const
	  {}
	  void EvaluateChart(const ChartHypothesis &hypo,
	                             ScoreComponentCollection* accumulator) const
	  {}

};

}


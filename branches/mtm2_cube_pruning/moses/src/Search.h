
#pragma once

#include <vector>
#include "TypeDef.h"

class HypothesisStack;
class Hypothesis;
class InputType;

class Search
{
public:
	virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const = 0;
	virtual const Hypothesis *GetBestHypothesis() const = 0;
	virtual void ProcessSentence() = 0;

	// Factory
	static Search *CreateSearch(const InputType &source, SearchAlgorithm searchAlgorithm);
};


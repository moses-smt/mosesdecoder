
#pragma once

#include <vector>

class HypothesisStack;
class Hypothesis;

class Search
{
public:
	virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const = 0;
	virtual const Hypothesis *GetBestHypothesis() const = 0;

};


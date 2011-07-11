
#pragma once

#include <vector>
#include "TypeDef.h"
#include "Phrase.h"

namespace Moses
{

class HypothesisStack;
class Hypothesis;
class InputType;
class TranslationOptionCollection;

class Search
{
public:
	virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const = 0;
	virtual const Hypothesis *GetBestHypothesis() const = 0;
	virtual void ProcessSentence() = 0;
	Search();
	virtual ~Search()
	{}

	// Factory
	static Search *CreateSearch(const InputType &source, SearchAlgorithm searchAlgorithm, const TranslationOptionCollection &transOptColl);

protected:
	
	const Phrase *m_constraint;

};


}

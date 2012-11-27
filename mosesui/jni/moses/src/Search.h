#ifndef moses_Search_h
#define moses_Search_h

#include <vector>
#include "TypeDef.h"
#include "Phrase.h"

namespace Moses
{

class HypothesisStack;
class Hypothesis;
class InputType;
class TranslationOptionCollection;
class Manager;

class Search
{
public:
	virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const = 0;
	virtual const Hypothesis *GetBestHypothesis() const = 0;
	virtual void ProcessSentence() = 0;
  Search(Manager& manager) : m_manager(manager) {}
	virtual ~Search()
	{}

	// Factory
	static Search *CreateSearch(Manager& manager, const InputType &source, SearchAlgorithm searchAlgorithm, 
                              const TranslationOptionCollection &transOptColl);

protected:
	
	const Phrase *m_constraint;
  Manager& m_manager;

};


}
#endif

#pragma once

#include <map>
#include <vector>
#include <string>
#include "PhraseDictionary.h"
#include "ChartRuleCollection.h"
#include "PhraseDictionaryMemory.h"

namespace Moses
{

class ChartRuleCollection;

	// hack. just use phrase dict mem to store it
class PhraseDictionaryGlueRule : public PhraseDictionaryMemory
{
protected:
	typedef PhraseDictionaryMemory MyBase;
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryGlueRule&);

public:
	PhraseDictionaryGlueRule(size_t numScoreComponent);

	~PhraseDictionaryGlueRule()
	{}

	std::string GetScoreProducerDescription() const
	{ return "Glue Rule"; }

	PhraseTableImplementation GetPhraseTableImplementation() const
	{ return GlueRule; }

	bool Load(const std::vector<FactorType> &input
					, const std::vector<FactorType> &output
					, const std::vector<float> &weight
					, size_t tableLimit
					, const LMList &languageModels
					, float weightWP);
};

} // namespace


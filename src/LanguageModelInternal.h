
#pragma once

#include "LanguageModelSingleFactor.h"
#include "NGramCollection.h"

class LanguageModelInternal : public LanguageModelSingleFactor
{
protected:
	NGramCollection m_map;
public:
	LanguageModelInternal(bool registerScore);
	void Load(const std::string &filePath
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	float GetValue(const std::vector<const Word*> &contextFactor
												, State* finalState = 0
												, unsigned int* len = 0) const;
};


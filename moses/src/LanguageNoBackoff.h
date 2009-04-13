
#pragma once

#include "LanguageModelSingleFactor.h"
#include "NGramCollection.h"

/** Guaranteed cross-platform LM implementation designed to mimic LM used in regression tests
*/
class LanguageNoBackoff : public LanguageModelSingleFactor
{
protected:
	std::vector<const NGramNode*> m_lmIdLookup;
	NGramCollection m_map;

	const NGramNode* GetLmID( const Factor *factor ) const
	{
		size_t factorId = factor->GetId();
		return ( factorId >= m_lmIdLookup.size()) ? NULL : m_lmIdLookup[factorId];        
  };

public:
	LanguageNoBackoff(bool registerScore, ScoreIndexManager &scoreIndexManager);
	bool Load(const std::string &filePath
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	float GetValue(const std::vector<const Word*> &contextFactor
												, State* finalState = 0
												, unsigned int* len = 0) const;
};


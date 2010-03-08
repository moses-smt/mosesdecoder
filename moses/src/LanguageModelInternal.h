#ifndef moses_LanguageModelInternal_h
#define moses_LanguageModelInternal_h

#include "LanguageModelSingleFactor.h"
#include "NGramCollection.h"

namespace Moses
{

/** Guaranteed cross-platform LM implementation designed to mimic LM used in regression tests
*/
class LanguageModelInternal : public LanguageModelSingleFactor
{
protected:
	std::vector<const NGramNode*> m_lmIdLookup;
	NGramCollection m_map;

	const NGramNode* GetLmID( const Factor *factor ) const
	{
		size_t factorId = factor->GetId();
		return ( factorId >= m_lmIdLookup.size()) ? NULL : m_lmIdLookup[factorId];        
  };

	float GetValue(const Factor *factor0, State* finalState) const;
	float GetValue(const Factor *factor0, const Factor *factor1, State* finalState) const;
	float GetValue(const Factor *factor0, const Factor *factor1, const Factor *factor2, State* finalState) const;

public:
	LanguageModelInternal(bool registerScore, ScoreIndexManager &scoreIndexManager);
	bool Load(const std::string &filePath
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	float GetValue(const std::vector<const Word*> &contextFactor
												, State* finalState = 0
												, unsigned int* len = 0) const;
};

}

#endif

#pragma once

/*
 *  Word.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <vector>
#include "../../moses/src/TypeDef.h"
#include "Vocab.h"

namespace Moses
{
	class Word;
}

namespace MosesBerkeleyPt
{

class Vocab;
	
class Word
{
	bool m_isNonTerminal;
	std::vector<VocabId> m_factors;
public:
	Word();
	virtual ~Word();
	Word(size_t numFactors)
	:m_factors(numFactors)
	{}
	
	void CreateFromString(const std::string &inString, Vocab &vocab);
	
	VocabId GetVocabId(size_t ind) const
	{ return m_factors[ind]; }
	void SetVocabId(size_t ind, VocabId vocabId)
	{ m_factors[ind] = vocabId; }

	bool IsNonTerminal() const
	{ return m_isNonTerminal; }
	
	size_t WriteToMemory(char *mem) const;
	size_t ReadFromMemory(const char *mem, size_t numFactors);
	
	Moses::Word *ConvertToMoses(Moses::FactorDirection direction
														, const std::vector<Moses::FactorType> &outputFactorsVec
														, const Vocab &vocab) const;
	
};

}; // namespace


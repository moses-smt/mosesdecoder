#pragma once
/*
 *  Word.h
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Vocab.h"

namespace Moses
{
	class Word;
}

namespace OnDiskPt
{

class Word
{
	friend std::ostream& operator<<(std::ostream&, const Word&);

protected:
	bool m_isNonTerminal;
	std::vector<UINT64> m_factors;
	
public:
	explicit Word()
	{}
	
	explicit Word(size_t numFactors, bool isNonTerminal)
	:m_factors(numFactors)
	,m_isNonTerminal(isNonTerminal)
	{}
	
	Word(const Word &copy);
	~Word();
	

	void CreateFromString(const std::string &inString, Vocab &vocab);
	bool IsNonTerminal() const
	{ return m_isNonTerminal; }

	size_t WriteToMemory(char *mem) const;
	size_t ReadFromMemory(const char *mem, size_t numFactors);
	size_t ReadFromFile(std::fstream &file, size_t numFactors);

	void SetVocabId(size_t ind, UINT32 vocabId)
	{ m_factors[ind] = vocabId; }

	Moses::Word *ConvertToMoses(Moses::FactorDirection direction
															, const std::vector<Moses::FactorType> &outputFactorsVec
															, const Vocab &vocab) const;
	
	int Compare(const Word &compare) const;
	bool operator<(const Word &compare) const;
	bool operator==(const Word &compare) const;

};
}


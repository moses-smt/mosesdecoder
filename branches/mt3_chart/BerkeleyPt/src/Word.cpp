/*
 *  Word.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Word.h"
#include "../../moses/src/Util.h"

using namespace std;

namespace MosesBerkeleyPt
{

void Word::CreateFromString(const std::string &inString, Vocab &vocab)
{
	string str = inString;
	if (str.substr(0, 1) == "[" && str.substr(str.size() - 1, 1) == "]")
	{ // non-term
		str = str.substr(1, str.size() - 2);
		m_isNonTerminal = true;
	}
	else
	{
		m_isNonTerminal = false;
	}
	
	std::vector<string> factorsStr = Moses::Tokenize(str, "|");
	m_factors.resize(factorsStr.size());
	cerr << vocab.GetSize();
	
	for (size_t ind = 0; ind < factorsStr.size(); ++ind)
	{
		m_factors[ind] = vocab.AddVocabId(factorsStr[ind]);
		cerr << vocab.GetSize();
	}
	
}

size_t Word::WriteToMemory(char *mem) const
{	
	VocabId *vocabMem = (VocabId*) mem;
	
	// factors
	for (int ind = 0; ind < m_factors.size(); ind++)
		vocabMem[ind] = m_factors[ind];
	
	size_t size = sizeof(VocabId) * m_factors.size();

	// is no-term
	char bNonTerm = (char) m_isNonTerminal;
	mem[size] = bNonTerm;

	++size;
	return size;
}

size_t Word::ReadFromMemory(const char *mem, size_t numFactors)
{
	m_factors.resize(numFactors);

	VocabId *vocabMem = (VocabId*) mem;
	
	for (int ind = 0; ind < m_factors.size(); ind++)
		m_factors[ind] = vocabMem[ind];

	size_t size = sizeof(VocabId) * m_factors.size();

	m_isNonTerminal = (bool) mem[size];

	++size;
	return size;
}

}; // namespace


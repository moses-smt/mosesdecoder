/*
 *  Word.cpp
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "../../moses/src/Util.h"
#include "../../moses/src/Word.h"
#include "Word.h"

using namespace std;

namespace OnDiskPt
{

Word::Word(const Word &copy)
	:m_isNonTerminal(copy.m_isNonTerminal)
	,m_factors(copy.m_factors)
{}
	
Word::~Word()
{}

void Word::CreateFromString(const std::string &inString, Vocab &vocab)
{
	if (inString.substr(0, 1) == "[" && inString.substr(inString.size() - 1, 1) == "]")
	{ // non-term
		m_isNonTerminal = true;
	}
	else
	{
		m_isNonTerminal = false;
	}

	m_factors.resize(1);
	m_factors[0] = vocab.AddVocabId(inString);	
}

size_t Word::WriteToMemory(char *mem) const
{	
	UINT64 *vocabMem = (UINT64*) mem;
	
	// factors
	for (size_t ind = 0; ind < m_factors.size(); ind++)
		vocabMem[ind] = m_factors[ind];
	
	size_t size = sizeof(UINT64) * m_factors.size();

	// is non-term
	char bNonTerm = (char) m_isNonTerminal;
	mem[size] = bNonTerm;
	++size;

	return size;
}

size_t Word::ReadFromMemory(const char *mem, size_t numFactors)
{
	m_factors.resize(numFactors);
	UINT64 *vocabMem = (UINT64*) mem;
	
	// factors
	for (size_t ind = 0; ind < m_factors.size(); ind++)
		m_factors[ind] = vocabMem[ind];
	
	size_t memUsed = sizeof(UINT64) * m_factors.size();
	
	// is non-term
	char bNonTerm;
	bNonTerm = mem[memUsed];
	m_isNonTerminal = (bool) bNonTerm;
	++memUsed;
	
	return memUsed;	
}

size_t Word::ReadFromFile(std::fstream &file, size_t numFactors)
{
	size_t memAlloc = numFactors * sizeof(UINT64) + sizeof(char);
	char *mem = (char*) malloc(memAlloc);
	file.read(mem, memAlloc);
	
	size_t memUsed = ReadFromMemory(mem, numFactors);
	assert(memAlloc == memUsed);
	free(mem);
	
	return memUsed;
}
	
Moses::Word *Word::ConvertToMoses(Moses::FactorDirection direction
														, const std::vector<Moses::FactorType> &outputFactorsVec
														, const Vocab &vocab) const
{
	Moses::Word *ret = new Moses::Word(m_isNonTerminal);
	
	for (size_t ind = 0; ind < m_factors.size(); ++ind)
	{
		Moses::FactorType factorType = outputFactorsVec[ind];
		UINT32 vocabId = m_factors[ind];
		const Moses::Factor *factor = vocab.GetFactor(vocabId, factorType, direction, m_isNonTerminal);
		ret->SetFactor(factorType, factor);
	}
	
	return ret;
	
}

int Word::Compare(const Word &compare) const
{
	int ret;
	
	if (m_isNonTerminal != compare.m_isNonTerminal)
		return m_isNonTerminal ?-1 : 1;
	
	if (m_factors < compare.m_factors)
		ret = -1;
	else if (m_factors > compare.m_factors)
		ret = 1;
	else
		ret = 0;

	return ret;
}

bool Word::operator<(const Word &compare) const
{ 
	int ret = Compare(compare);
	return ret < 0;
}

bool Word::operator==(const Word &compare) const
{ 
	int ret = Compare(compare);
	return ret == 0;
}
	
std::ostream& operator<<(std::ostream &out, const Word &word)
{
	out << "[";
	
	std::vector<UINT64>::const_iterator iter;
	for (iter = word.m_factors.begin(); iter != word.m_factors.end(); ++iter)
	{
		out << *iter << "|";
	}
	
	out << (word.m_isNonTerminal ? "n" : "t");
	out << "]";
	
	return out;
}
}

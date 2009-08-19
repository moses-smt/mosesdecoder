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
#include "../../moses/src/Word.h"

using namespace std;

namespace MosesBerkeleyPt
{
Word::Word()
{}

Word::~Word()
{}

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
	
	std::vector<string> factorsStr;
	Moses::Tokenize(factorsStr, str, "|");
	m_factors.resize(factorsStr.size());
	
	for (size_t ind = 0; ind < factorsStr.size(); ++ind)
	{
		const string &str = factorsStr[ind];
		m_factors[ind] = vocab.AddVocabId(str);
	}
	
}

size_t Word::WriteToMemory(char *mem) const
{	
	Moses::UINT32 *vocabMem = (Moses::UINT32*) mem;
	
	// factors
	for (int ind = 0; ind < m_factors.size(); ind++)
		vocabMem[ind] = m_factors[ind];
	
	size_t size = sizeof(Moses::UINT32) * m_factors.size();

	// is no-term
	char bNonTerm = (char) m_isNonTerminal;
	mem[size] = bNonTerm;
	++size;

	return size;
}

size_t Word::ReadFromMemory(const char *mem, size_t numFactors)
{
	m_factors.resize(numFactors);

	Moses::UINT32 *vocabMem = (Moses::UINT32*) mem;
	
	for (int ind = 0; ind < m_factors.size(); ind++)
		m_factors[ind] = vocabMem[ind];

	size_t size = sizeof(Moses::UINT32) * m_factors.size();
	m_isNonTerminal = (bool) mem[size];
	++size;

	return size;
}

Moses::Word *Word::ConvertToMoses(Moses::FactorDirection direction
																	,const std::vector<Moses::FactorType> &outputFactorsVec
																	, const Vocab &vocab) const
{
	Moses::Word *ret = new Moses::Word(m_isNonTerminal);

	for (size_t ind = 0; ind < m_factors.size(); ++ind)
	{
		Moses::FactorType factorType = outputFactorsVec[ind];
		Moses::UINT32 vocabId = m_factors[ind];
		const Moses::Factor *factor = vocab.GetFactor(vocabId, factorType, direction, IsNonTerminal());
		ret->SetFactor(factorType, factor);
	}
	
	return ret;
}
	
bool Word::operator<(const Word &compare) const
{ 
	if (m_isNonTerminal != compare.m_isNonTerminal)
		return m_isNonTerminal < compare.m_isNonTerminal;
	
	return m_factors < compare.m_factors;
}
	

std::ostream& operator<<(std::ostream &out, const Word &word)
{
	out << "[";

	std::vector<Moses::UINT32>::const_iterator iter;
	for (iter = word.m_factors.begin(); iter != word.m_factors.end(); ++iter)
	{
		out << *iter << "|";
	}
	out << "]";

	return out;
}


}; // namespace


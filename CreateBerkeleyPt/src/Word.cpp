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

void Word::CreateFromString(const std::string &inString)
{
	Vocab &vocab = Vocab::Instance();
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
	
	for (size_t ind = 0; ind < factorsStr.size(); ++ind)
	{
		m_factors[ind] = vocab.AddFactor(factorsStr[ind]);
	}
	
}

size_t Word::WriteToMemory(char *mem) const
{
	size_t size = sizeof(VocabId) * m_factors.size();
	memcpy(mem, &m_factors, size);
	
	return size;
}



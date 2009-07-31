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
#include "Vocab.h"

namespace MosesBerkeleyPt
{

class Word
{
	bool m_isNonTerminal;
	std::vector<VocabId> m_factors;
public:
	void CreateFromString(const std::string &inString);
	
	VocabId GetFactor(size_t ind) const
	{ return m_factors[ind]; }
	bool IsNonTerminal() const
	{ return m_isNonTerminal; }
	
	size_t WriteToMemory(char *mem) const;
};

}; // namespace


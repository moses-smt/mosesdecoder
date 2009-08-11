#pragma once

/*
 *  Phrase.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <vector>
#include "Word.h"

class Db;

namespace MosesBerkeleyPt
{

class Phrase
{
protected:
	std::vector<Word>	m_words;
		
public:
	Phrase();
	~Phrase();
	
	void CreateFromString(const std::string &phraseString, Vocab &vocab);

	size_t GetSize() const
	{ return m_words.size(); }
	const Word &GetWord(size_t pos) const
	{ return m_words[pos]; }
	Word &GetWord(size_t pos)
	{ return m_words[pos]; }

	void Resize(size_t newSize)
	{ m_words.resize(newSize); }
	
	//! transitive comparison
	inline bool operator<(const Phrase &compare) const
	{
		return m_words < compare.m_words;
	}
	
};

}; // namespace

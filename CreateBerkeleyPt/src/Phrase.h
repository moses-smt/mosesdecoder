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

class Phrase
{
protected:
	std::vector<Word>	m_words;
	
	typedef std::pair<size_t, size_t>  AlignPair;
	typedef std::vector<AlignPair> AlignType;
	AlignType m_align;
	std::vector<float>				m_scores;
	std::vector<Word>	m_headWords;
public:
	void CreateFromString(const std::string &phraseString);
	void CreateAlignFromString(const std::string &alignString);
	void CreateScoresFromString(const std::string &inString);
	void CreateHeadwordsFromString(const std::string &inString);

	size_t GetSize() const
	{ return m_words.size(); }
	const Word &GetWord(size_t pos) const
	{ return m_words[pos]; }

	const std::vector<Word> &GetHeadWords() const
	{ return m_headWords; }

	
	const AlignType &GetAlign() const
	{ return m_align; }
	size_t GetAlign(size_t sourcePos) const;
};

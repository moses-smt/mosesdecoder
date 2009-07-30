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

class Phrase
{
protected:
	std::vector<Word>	m_words;
	
	typedef std::pair<int, int>  AlignPair;
	typedef std::vector<AlignPair> AlignType;
	AlignType m_align;
	std::vector<float>				m_scores;
	std::vector<Word>	m_headWords;
	
	char *WriteToMemory() const;
	size_t WriteAlignToMemory(char *mem) const;
	size_t WriteScoresToMemory(char *mem) const;

public:
	void CreateFromString(const std::string &phraseString);
	void CreateAlignFromString(const std::string &alignString);
	void CreateScoresFromString(const std::string &inString);
	void CreateHeadwordsFromString(const std::string &inString);

	size_t GetSize() const
	{ return m_words.size(); }
	const Word &GetWord(size_t pos) const
	{ return m_words[pos]; }

	const Word &GetHeadWords(size_t ind) const
	{ return m_headWords[ind]; }

	
	const AlignType &GetAlign() const
	{ return m_align; }
	size_t GetAlign(size_t sourcePos) const;
	

	void SaveTargetPhrase(Db &db) const;

};

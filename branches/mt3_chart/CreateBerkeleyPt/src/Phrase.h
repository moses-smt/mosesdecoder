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
	std::vector<std::pair<size_t, size_t> > m_align;
	std::vector<float>				m_scores;
	std::vector<Word>	m_headWords;
public:
	void CreateFromString(const std::string &phraseString);
	void CreateAlignFromString(const std::string &alignString);
	void CreateScoresFromString(const std::string &inString);
	void CreateHeadwordsFromString(const std::string &inString);

};

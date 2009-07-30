/*
 *  Phrase.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Phrase.h"
#include "../../moses/src/Util.h"

using namespace std;

void Phrase::CreateFromString(const std::string &phraseString)
{
	std::vector<std::string> wordsVec = Moses::Tokenize(phraseString);

	vector<string>::const_iterator iter;
	for (iter = wordsVec.begin(); iter != wordsVec.end(); ++iter)
	{
		const string &wordStr = *iter;
		Word word;
		word.CreateFromString(wordStr);
		m_words.push_back(word);
	}
}

void Phrase::CreateAlignFromString(const std::string &alignString)
{
	vector<string> alignTok = Moses::Tokenize(alignString);

	vector<string>::const_iterator iter;
	for (iter = alignTok.begin(); iter != alignTok.end(); ++iter)
	{
		const string &align1Str = *iter;
		vector<size_t> alignPoints = Moses::Tokenize<size_t>(align1Str, "-");
		assert(alignPoints.size() == 2);
		m_align.push_back(pair<size_t, size_t>(alignPoints[0], alignPoints[1]) );
	}
}

void Phrase::CreateScoresFromString(const std::string &inString)
{
	m_scores = Moses::Tokenize<float>(inString);
}

void Phrase::CreateHeadwordsFromString(const std::string &inString)
{
	std::vector<string> headWordsStr = Moses::Tokenize(inString);
	assert(headWordsStr.size());
	
	m_headWords.push_back(Word());
	m_headWords.push_back(Word());
	
	m_headWords[0].CreateFromString(headWordsStr[0]);
	m_headWords[1].CreateFromString(headWordsStr[1]);
}




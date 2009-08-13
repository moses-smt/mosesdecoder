/*
 *  Phrase.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <db_cxx.h>
#include "../../moses/src/Util.h"
#include "Phrase.h"

using namespace std;

namespace MosesBerkeleyPt
{
Phrase::Phrase()
{}

Phrase::Phrase(const Phrase &copy)
:m_words(copy.m_words)
{}

Phrase::~Phrase()
{}

void Phrase::CreateFromString(const std::string &phraseString, Vocab &vocab)
{
	vector<string> wordsVec;
	Moses::Tokenize(wordsVec, phraseString);

	vector<string>::const_iterator iter;
	for (iter = wordsVec.begin(); iter != wordsVec.end(); ++iter)
	{
		const string &wordStr = *iter;
		Word word;
		word.CreateFromString(wordStr, vocab);
		m_words.push_back(word);
	}
}

//! transitive comparison
bool Phrase::operator<(const Phrase &compare) const
{
	return m_words < compare.m_words;
}
	
}; // namespace




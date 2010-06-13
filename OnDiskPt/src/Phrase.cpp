/*
 *  Phrase.cpp
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <iostream>
#include <cassert>
#include "../../moses/src/Util.h"
#include "Phrase.h"

using namespace std;

namespace OnDiskPt
{

Phrase::Phrase(const Phrase &copy)
:m_words(copy.GetSize())
{
	for (size_t pos = 0; pos < copy.GetSize(); ++pos)
	{
		const Word &oldWord = copy.GetWord(pos);
		Word *newWord = new Word(oldWord);
		m_words[pos] = newWord;
	}
}

Phrase::~Phrase()
{
	Moses::RemoveAllInColl(m_words);
}

void Phrase::AddWord(Word *word)
{
	m_words.push_back(word);
}

void Phrase::AddWord(Word *word, size_t pos)
{
	assert(pos < m_words.size());
	m_words.insert(m_words.begin() + pos + 1, word);
}

int Phrase::Compare(const Phrase &compare) const
{
	int ret = 0;
	for (size_t pos = 0; pos < GetSize(); ++pos)
	{
		if (pos >= compare.GetSize())
		{ // we're bigger than the other. Put 1st
			ret = -1;
			break;
		}
		
		const Word &thisWord = GetWord(pos)
							,&compareWord = compare.GetWord(pos);
		int wordRet = thisWord.Compare(compareWord);
		if (wordRet != 0)
		{ 
			ret = wordRet;
			break;
		}
	}

	if (ret == 0)
	{
		assert(compare.GetSize() >= GetSize());
		ret = (compare.GetSize() > GetSize()) ? 1 : 0;
	}
	return ret;		
}

//! transitive comparison
bool Phrase::operator<(const Phrase &compare) const
{	
	int ret = Compare(compare);
	return ret < 0;
}

bool Phrase::operator>(const Phrase &compare) const
{	
	int ret = Compare(compare);
	return ret > 0;
}

bool Phrase::operator==(const Phrase &compare) const
{	
	int ret = Compare(compare);
	return ret == 0;
}

std::ostream& operator<<(std::ostream &out, const Phrase &phrase)
{
	for (size_t pos = 0; pos < phrase.GetSize(); ++pos)
	{
		const Word &word = phrase.GetWord(pos);
		out << word << " ";
	}
	
	return out;
}
	
}


#pragma once
/*
 *  Phrase.h
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include <iostream>
#include "Word.h"

namespace OnDiskPt
{

class Phrase
{
	friend std::ostream& operator<<(std::ostream&, const Phrase&);

protected:
	std::vector<Word*>	m_words;
	
public:
	Phrase() 
	{}
	Phrase(const Phrase &copy);
	virtual ~Phrase();
	
	void AddWord(Word *word);
	void AddWord(Word *word, size_t pos);

	const Word &GetWord(size_t pos) const
	{ return *m_words[pos]; }
	size_t GetSize() const
	{ return m_words.size(); }

	int Compare(const Phrase &compare) const;
	bool operator<(const Phrase &compare) const;
	bool operator>(const Phrase &compare) const;
	bool operator==(const Phrase &compare) const;
};

}

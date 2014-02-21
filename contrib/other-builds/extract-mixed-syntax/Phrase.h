#pragma once

#include <vector>
#include "Word.h"

class Phrase : public std::vector<Word*>
{
public:
	Phrase()
	{}

	Phrase(size_t size)
	:std::vector<Word*>(size)
	 {}

	void Debug(std::ostream &out) const;

};

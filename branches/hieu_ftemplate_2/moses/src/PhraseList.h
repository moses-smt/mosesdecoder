
#pragma once

#include <list>
#include "Phrase.h"

class PhraseList : public std::list<Phrase>
{
public:
	void Load(std::string filePath);

};


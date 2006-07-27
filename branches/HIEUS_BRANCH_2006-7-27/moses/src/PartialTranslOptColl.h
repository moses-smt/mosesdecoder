
#pragma once

#include <list>
#include "TranslationOption.h"

class PartialTranslOptColl : public std::list< TranslationOption >
{
public:
	void Add(const TranslationOption &partialTranslOpt)
	{
		push_back(partialTranslOpt);
	}
};

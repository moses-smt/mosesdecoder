
#pragma once

#include <list>
#include "TranslationOption.h"
#include "Util.h"

class PartialTranslOptColl : public std::list< TranslationOption* >
{
public:
	void Add(TranslationOption *partialTranslOpt)
	{
		push_back(partialTranslOpt);
	}
	~PartialTranslOptColl()
	{
		RemoveAllInColl<PartialTranslOptColl::iterator>(*this);
	}
	void DetachAll()
	{
		clear();
	}
};

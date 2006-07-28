
#pragma once

#include <list>
#include "TranslationOption.h"
#include "Util.h"

class PartialTranslOptColl : public std::list< TranslationOption* >
{
public:
	~PartialTranslOptColl()
	{
		RemoveAllInColl<PartialTranslOptColl::iterator>(*this);
	}
	
	void Add(TranslationOption *partialTranslOpt)
	{
		push_back(partialTranslOpt);
	}
	void DetachAll()
	{
		clear();
	}	
};

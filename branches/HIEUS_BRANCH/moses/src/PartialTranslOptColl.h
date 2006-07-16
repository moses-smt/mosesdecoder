
#pragma once

#include <list>
#include "PartialTranslOpt.h"

class PartialTranslOptColl : public std::list< PartialTranslOpt >
{
public:
	void Add(const PartialTranslOpt &partialTranslOpt)
	{
		push_back(partialTranslOpt);
	}
};

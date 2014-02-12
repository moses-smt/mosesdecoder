/*
 *  SymbolSequence.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 21/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <cassert>
#include <sstream>
#include "SymbolSequence.h"

using namespace std;

int SymbolSequence::Compare(const SymbolSequence &other) const
{	
	int ret;
	size_t thisSize = GetSize();
	size_t otherSize = other.GetSize();
	if (thisSize != otherSize)
	{
		ret = (thisSize < otherSize) ? -1 : +1;
		return ret;
	}
	else 
	{
		assert(thisSize == otherSize);
		for (size_t ind = 0; ind < thisSize; ++ind)
		{
			const Symbol &thisSymbol = GetSymbol(ind);
			const Symbol &otherSymbol = other.GetSymbol(ind);
			ret = thisSymbol.Compare(otherSymbol);
			if (ret != 0)
			{
				return ret;
			}
		}
	}
	
	assert(ret == 0);
	return ret;
}

std::ostream& operator<<(std::ostream &out, const SymbolSequence &obj)
{	
	SymbolSequence::CollType::const_iterator iterSymbol;
	for (iterSymbol = obj.m_coll.begin(); iterSymbol != obj.m_coll.end(); ++iterSymbol)
	{
		const Symbol &symbol = *iterSymbol;
		out << symbol << " ";
	}
	
	return out;
}
	


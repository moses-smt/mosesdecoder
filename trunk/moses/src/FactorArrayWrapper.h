// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <iostream>
#include "TypeDef.h"
#include "Factor.h"

class FactorArrayWrapper
{
	friend std::ostream& operator<<(std::ostream&, const FactorArrayWrapper&);
	
protected:
	const FactorArray *m_factorArrayPtr;
public:
	FactorArrayWrapper() {}
	FactorArrayWrapper(const FactorArray &factorArray)
		:m_factorArrayPtr(&factorArray) {}
	virtual ~FactorArrayWrapper();

	FactorArrayWrapper& operator=(const FactorArrayWrapper &other)
	{
		if(this != &other)
		{
			m_factorArrayPtr = other.m_factorArrayPtr;
		}
		return *this;
	}

	const Factor *operator[](size_t index) const
	{
		return (*m_factorArrayPtr)[index];
	}

	virtual const FactorArray &GetFactorArray() const
	{
		return *m_factorArrayPtr;
	}

	inline const Factor *GetFactor(FactorType factorType) const
	{
		return (*m_factorArrayPtr)[factorType];
	}

	int Compare(const FactorArrayWrapper &compare) const;
		// -1 = less than
		// +1 = more than
		// 0	= same
	
	inline bool operator< (const FactorArrayWrapper &compare) const
	{ // needed to store word in GenerationDictionary map
		// uses comparison of FactorKey
		// 'proper' comparison, not address/id comparison
		return Compare(compare) < 0;
	}

	TO_STRING;

	//statics
	static int Compare(const FactorArray &targetWord, const FactorArray &sourceWord);

};


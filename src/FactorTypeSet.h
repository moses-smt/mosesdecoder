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
#include <set>
#include <vector>
#include <algorithm>
#include <iterator>
#include "TypeDef.h"
#include "Util.h"

class FactorTypeSet
{
	friend std::ostream& operator<<(std::ostream&, const FactorTypeSet&);

protected:
	unsigned int m_bit;

	inline FactorTypeSet(unsigned int bit)
	{
		m_bit = bit;
	}
public:
	inline FactorTypeSet()
	:m_bit(0)
	{
	}
	FactorTypeSet (const std::vector<FactorType> &factors);

	inline void Add(const FactorType &factorType)
	{
		unsigned int value = static_cast<unsigned int>(factorType);
		m_bit |= (1 << value);
	}

	inline FactorTypeSet Intersect(const FactorTypeSet &other) const
	{
		unsigned int bit = m_bit & other.m_bit;
		return FactorTypeSet(bit);;
	}

	inline void Merge(const FactorTypeSet &other)
	{
		m_bit |= other.m_bit;
	}
	void Set(const FactorTypeSet &other)
	{
		m_bit = 0;
		Merge(other);
	}
	inline bool Contains(FactorType factorType) const
	{
		unsigned int value = static_cast<unsigned int>(factorType);
		return  (m_bit & value) > 0;
	}
	TO_STRING;	
};

// friend
inline
std::ostream& operator<<(std::ostream& out, const FactorTypeSet& factorTypeSet)
{
	out << "(";
	
	for (size_t currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
		if (factorTypeSet.Contains(currFactor))
		{
			out << "," << currFactor;
		}
	}
	out << ") ";
	
	return out;
}

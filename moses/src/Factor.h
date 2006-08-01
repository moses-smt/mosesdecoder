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

#include <sstream>
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <string>
#include "TypeDef.h"
#include "Util.h"
#include "hash.h"

class FactorCollection;

/** Represents a factor (word, POS, etc) on the E or F side
 * 
 * A Factor object is a tuple of direction (Input or Output,
 * corresponding to French or English), a type (surface form,
 * POS, stem, etc), and the value of the factor.
 *
 * @TODO I find this design problematic- essentially, a factor should
 * just be a value type and the factor type and "direction"
 * should be the keys in a larger identification system that
 * find instances of specific factors.
 *
 */
class Factor
{
	friend std::ostream& operator<<(std::ostream&, const Factor&);

	// only these classes are allowed to instantiate this class
	friend class FactorCollection;

protected:
	FactorDirection		m_direction;
	FactorType				m_factorType;
	const std::string	*m_ptrString;
	LmId 							m_lmId;

	Factor(FactorDirection direction, FactorType factorType, const std::string *factorString, LmId lmId);
	
	inline void SetLmId(LmId lmId)
	{
		m_lmId = lmId;
	}

public:
	inline size_t hash() const
	{
		size_t h=quick_hash((const char*)&m_direction, sizeof(FactorDirection), 0xc7e7f2fd);
		h=quick_hash((const char*)&m_factorType, sizeof(FactorType), h);
		h=quick_hash((const char*)&m_ptrString, sizeof(const std::string *), h);
		return h;
	}

	inline FactorDirection GetFactorDirection() const
	{
		return m_direction;
	}
	inline FactorType GetFactorType() const
	{
		return m_factorType;
	}
	inline const std::string &GetString() const
	{
		return *m_ptrString;
	}
	inline LmId GetLmId() const
	{
		return m_lmId;
	}
	
	// do it properly. needed for insert & finding of words in dictionary
	inline int Compare(const Factor &compare) const
	{
		if (m_ptrString < compare.m_ptrString)
			return -1;
		if (m_ptrString > compare.m_ptrString)
			return 1;

		if (m_direction < compare.m_direction)
			return -1;
		if (m_direction > compare.m_direction)
			return 1;

		if (m_factorType < compare.m_factorType)
			return -1;
		if (m_factorType > compare.m_factorType)
			return 1;

		return 0;
	}
	
	inline bool operator<(const Factor &compare) const
	{ 
		return Compare(compare) < 0;
	}

	inline bool operator==(const Factor &compare) const
	{ 
		return Compare(compare) == 0;
	}
	
	TO_STRING;

};


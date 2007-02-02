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

/** Represents a factor (word, POS, etc) on the E or F side
 * 
 * A Factor object is a tuple of direction (Input or Output,
 * corresponding to French or English), a type (surface form,
 * POS, stem, etc), and the value of the factor.
 *
 */
class Factor
{
	friend std::ostream& operator<<(std::ostream&, const Factor&);

	// only these classes are allowed to instantiate this class
	friend class FactorCollection;

protected:
	static size_t			s_id;

	FactorDirection		m_direction;
	FactorType				m_factorType;
	const std::string	*m_ptrString;
	const size_t			m_id;

	//! protected constructor. only friend class, FactorCollection, is allowed to create Factor objects
	Factor(FactorDirection direction, FactorType factorType, const std::string *factorString);
	
public:
	//! returns whether this factor is part of the source ('Input') or target ('Output') language
	inline FactorDirection GetFactorDirection() const
	{
		return m_direction;
	}
	//! index, FactorType. For example, 0=surface, 1=POS. The actual mapping is user defined
	inline FactorType GetFactorType() const
	{
		return m_factorType;
	}
	//! original string representation of the factor
	inline const std::string &GetString() const
	{
		return *m_ptrString;
	}
	//! contiguous ID
	inline size_t GetId() const
	{
		return m_id;
	}

	/*
	//! Alternative comparison between factors. Not yet used
	inline unsigned int GetHash() const
	{
		unsigned int h=quick_hash((const char*)&m_direction, sizeof(FactorDirection), 0xc7e7f2fd);
		h=quick_hash((const char*)&m_factorType, sizeof(FactorType), h);
		h=quick_hash((const char*)&m_ptrString, sizeof(const std::string *), h);
		return h;
	}
	*/
	
	/** transitive comparison between 2 factors.
	*	-1 = less than
	*	+1 = more than
	*	0	= same
	*	Used by operator< & operator==, as well as other classes
	*/
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
	//! transitive comparison used for adding objects into FactorCollection
	inline bool operator<(const Factor &compare) const
	{ 
		return Compare(compare) < 0;
	}

	// quick equality comparison. Not used
	inline bool operator==(const Factor &compare) const
	{ 
		return this == &compare;
	}

	TO_STRING();

};


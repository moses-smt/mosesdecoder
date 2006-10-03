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
#include <vector>
#include <list>
#include "TypeDef.h"
#include "Factor.h"
#include "Util.h"

class Phrase;

/***
 * hold a set of factors for a single word
 */
class Word
{
	friend std::ostream& operator<<(std::ostream&, const Word&);

protected:

	typedef const Factor * FactorArray[MAX_NUM_FACTORS];

	FactorArray m_factorArray;

public:
	/** deep copy */
	Word(const Word &copy) {
		std::memcpy(m_factorArray, copy.m_factorArray, sizeof(FactorArray));
	}

	/** empty word */
	Word() {
		std::memset(m_factorArray, 0, sizeof(FactorArray));
	}

	~Word() {}

	const Factor*& operator[](FactorType index) {
		return m_factorArray[index];
	}

	const Factor * const & operator[](FactorType index) const {
		return m_factorArray[index];
	}
	inline const Factor* GetFactor(FactorType factorType) const {
		return m_factorArray[factorType];
	}
	inline void SetFactor(FactorType factorType, const Factor *factor)
	{
		m_factorArray[factorType] = factor;
	}

	/** add the factors from sourceWord into this representation,
	 * NULL elements in sourceWord will be skipped */
	void Merge(const Word &sourceWord);

	std::string ToString(const std::vector<FactorType> factorType) const;
	TO_STRING;

	/* static functions */
	
	/***
	 * wherever the source word has a given factor that the target word is missing, add it to the target word
	 */
	static int Compare(const Word &targetWord, const Word &sourceWord);

        inline bool operator< (const Word &compare) const
        { // needed to store word in GenerationDictionary map
                // uses comparison of FactorKey
                // 'proper' comparison, not address/id comparison
                return Compare(*this, compare) < 0;
        }

};

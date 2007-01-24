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
class FactorMask;

/***
 * hold a set of factors for a single word
 */
class Word
{
	friend std::ostream& operator<<(std::ostream&, const Word&);

protected:

	typedef const Factor * FactorArray[MAX_NUM_FACTORS];

	FactorArray m_factorArray; /**< set of factors */

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

	//! returns Factor pointer for particular FactorType
	const Factor*& operator[](FactorType index) {
		return m_factorArray[index];
	}

	const Factor * const & operator[](FactorType index) const {
		return m_factorArray[index];
	}

	/** add the factors from sourceWord into this representation,
	 * NULL elements in sourceWord will be skipped */
	void Merge(const Word &sourceWord);

	//! keep only factors in inputMask, NULL all other factors
	void TrimFactors(const FactorMask &inputMask);

	/** get string representation of list of factors. Used by PDTimp so supposed 
	* to be invariant to changes in format of debuggin output, therefore, doesn't 
	* use streaming output or ToString() from any class so not dependant on 
	* these debugging functions.
	*/
	std::string GetString(const std::vector<FactorType> factorType,bool endWithBlank) const;
	TO_STRING();

	//! transitive comparison of Word objects
  inline bool operator< (const Word &compare) const
  { // needed to store word in GenerationDictionary map
          // uses comparison of FactorKey
          // 'proper' comparison, not address/id comparison
          return Compare(*this, compare) < 0;
  }

	/* static functions */
	
	/** transitive comparison of 2 word objects. Used by operator<. 
	*	Only compare the ?co-joined? factors, ie. where factor exists for both words.
	*	Should make it non-static
	*/
	static int Compare(const Word &targetWord, const Word &sourceWord);

};

struct WordComparer
{
	//! returns true if hypoA can be recombined with hypoB
	bool operator()(const Word *a, const Word *b) const
	{
		return *a < *b;
	}
};


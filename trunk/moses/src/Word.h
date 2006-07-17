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
#include "FactorTypeSet.h"
#include "Util.h"

class Phrase;

class Word
{
	friend std::ostream& operator<<(std::ostream&, const Word&);

protected:
	FactorArray					m_factorArray;

public:
	Word(const Word &copy);
	//Word(const FactorTypeSet *factorsUsed);
	Word();

	~Word()
	{
	}

	inline FactorArray &GetFactorArray()
	{
		return m_factorArray;
	}
	inline const FactorArray &GetFactorArray() const
	{
		return m_factorArray;
	}
	inline const Factor *GetFactor(FactorType factorType) const
	{
		return m_factorArray[factorType];
	}
	inline void SetFactor(FactorType factorType, const Factor *factor)
	{
		m_factorArray[factorType] = factor;
	}

	int Compare(const Word &compare) const;
		// -1 = less than
		// +1 = more than
		// 0	= same
	
	inline bool operator< (const Word &compare) const
	{ // needed to store word in GenerationDictionary map
		// uses comparison of FactorKey
		// 'proper' comparison, not address/id comparison
		return Compare(compare) < 0;
	}

	TO_STRING

	// FactorArray
	static void Copy(FactorArray &target, const FactorArray &source);
	static void Initialize(FactorArray &factorArray);
	/***
	 * wherever the source word has a given factor that the target word is missing, add it to the target word
	 */
	static void Merge(FactorArray &targetWord, const FactorArray &sourceWord);
	static std::string ToString(const FactorArray &factorArray);
	static int Compare(const FactorArray &targetWord, const FactorArray &sourceWord);

};


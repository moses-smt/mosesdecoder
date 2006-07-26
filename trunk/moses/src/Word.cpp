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

#include <sstream>
#include "memory.h"
#include "Word.h"
#include "TypeDef.h"

using namespace std;

Word::Word(const Word &copy)
{ // automatically points to orig's factors
	Word::Copy(m_factorArray, copy.m_factorArray);
}

Word::Word()
{
	Word::Initialize(m_factorArray);
}

Word::Word(const FactorArray &factorArray)
{
	for (size_t factor = 0 ; factor < NUM_FACTORS ; factor++)
	{
		m_factorArray[factor] = factorArray[factor];
	}
}

int Word::Compare(const Word &compare) const
{
	return Compare(GetFactorArray(), compare.GetFactorArray());
}

// static
int Word::Compare(const FactorArray &targetWord, const FactorArray &sourceWord)
{
	for (size_t factorType = 0 ; factorType < NUM_FACTORS ; factorType++)
	{
		const Factor *targetFactor		= targetWord[factorType]
								,*sourceFactor	= sourceWord[factorType];
					
		if (targetFactor == NULL || sourceFactor == NULL)
		{
			continue;
		}
		int result = targetFactor->Compare(*sourceFactor);
		if ( result )
			return result;
	}
	return 0;

}

void Word::Copy(FactorArray &target, const FactorArray &source)
{
	memcpy(target, source, sizeof(FactorArray));
}

void Word::Initialize(FactorArray &factorArray)
{
	memset(factorArray, 0, sizeof(FactorArray));
}

void Word::Merge(FactorArray &targetWord, const FactorArray &sourceWord)
{
	for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
		const Factor *sourcefactor		= sourceWord[currFactor]
								,*targetFactor			= targetWord[currFactor];
		if (targetFactor == NULL && sourcefactor != NULL)
		{
			targetWord[currFactor] = sourcefactor;
		}
	}
}

std::string Word::ToString(const FactorArray &factorArray)
{
	stringstream strme;

	for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
			const Factor *factor = factorArray[currFactor];
		if (factor != NULL)
		{
			strme << *factor << "|";
		}
	}
	string str = strme.str();
	str = str.substr(0, str.size() - 1) + " ";
	return str;
}

// friend
ostream& operator<<(ostream& out, const Word& word)
{	
	stringstream strme;

//	strme << "(";
	for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
		FactorType factorType = static_cast<FactorType>(currFactor);
		const Factor *factor = word.GetFactor(factorType);
		if (factor != NULL)
		{
			strme << *factor << "|";
		}
	}
	string str = strme.str();
	str = str.substr(0, str.size() - 1);
	out << str << " ";
	return out;
}


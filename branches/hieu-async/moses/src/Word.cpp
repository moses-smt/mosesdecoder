// $Id$
// vim::tabstop=2

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
#include "StaticData.h"  // needed to determine the FactorDelimiter
#include "FactorMask.h"

using namespace std;

// static
int Word::Compare(const Word &targetWord, const Word &sourceWord)
{
	for (FactorType factorType = 0 ; factorType < MAX_NUM_FACTORS ; factorType++)
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

void Word::Merge(const Word &sourceWord)
{
	for (FactorType currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++)
	{
		const Factor *sourcefactor		= sourceWord.m_factorArray[currFactor]
								,*targetFactor		= this     ->m_factorArray[currFactor];
		if (targetFactor == NULL && sourcefactor != NULL)
		{
			m_factorArray[currFactor] = sourcefactor;
		}
	}
}

void Word::TrimFactors(const FactorMask &inputMask)
{
	for (FactorType currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++)
	{
		if (!inputMask[currFactor])
			operator[](currFactor) = NULL;
	}
}

std::string Word::GetString(const vector<FactorType> factorType,bool endWithBlank) const
{
	stringstream strme;
	assert(factorType.size() <= MAX_NUM_FACTORS);
	const std::string& factorDelimiter = StaticData::Instance()->GetFactorDelimiter();
	bool firstPass = true;
	for (unsigned int i = 0 ; i < factorType.size() ; i++)
	{
		const Factor *factor = m_factorArray[factorType[i]];
		if (factor != NULL)
		{
			if (firstPass) { firstPass = false; } else { strme << factorDelimiter; }
			strme << factor->GetString();
		}
	}
	if(endWithBlank) strme << " ";
	return strme.str();
}

TO_STRING_BODY(Word);

// friend
ostream& operator<<(ostream& out, const Word& word)
{	
	stringstream strme;

	const std::string& factorDelimiter = StaticData::Instance()->GetFactorDelimiter();
	bool firstPass = true;
	for (FactorType factorType = 0 ; factorType < MAX_NUM_FACTORS ; ++factorType)
	{
		const Factor *factor = word[factorType];
		if (factor != NULL)
		{
			if (firstPass) { firstPass = false; } else { strme << factorDelimiter; }
			strme << *factor;
		}
	}
	out << strme.str() << " ";
	return out;
}

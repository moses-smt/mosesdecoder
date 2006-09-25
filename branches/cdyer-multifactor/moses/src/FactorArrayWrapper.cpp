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

#include "FactorArrayWrapper.h"
#include "Util.h"
#include "Word.h"

using namespace std;

FactorArrayWrapper::~FactorArrayWrapper() {}

int FactorArrayWrapper::Compare(const FactorArrayWrapper &compare) const
{
	return Compare(GetFactorArray(), compare.GetFactorArray());
}

// static functions
int FactorArrayWrapper::Compare(const FactorArray &targetWord, const FactorArray &sourceWord)
{
	for (size_t factorType = 0 ; factorType < MAX_NUM_FACTORS ; factorType++)
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

TO_STRING_BODY(FactorArrayWrapper);

// friend
ostream& operator<<(ostream& out, const FactorArrayWrapper& wrapper)
{	
	out << Word::ToString(*wrapper.m_factorArrayPtr);
	return out;
}

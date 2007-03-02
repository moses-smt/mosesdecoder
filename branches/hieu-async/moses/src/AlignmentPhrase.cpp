// $Id: AlignmentPhrase.cpp 988 2006-11-21 19:35:37Z hieuhoang1972 $
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

#include "AlignmentPhrase.h"
#include "WordsRange.h"
#include "WordsBitmap.h"

using namespace std;

bool AlignmentPhrase::IsCompatible(const AlignmentPhrase &compare, size_t shiftCompare) const 
{
	const size_t endPos = std::min(GetSize() , shiftCompare + compare.GetSize());
	size_t posCompare = 0;
	for (size_t posThis = shiftCompare ; posThis < endPos ; ++posThis)
	{
		const AlignmentElement &alignThis = m_collection[posThis];
		AlignmentElement alignCompare = compare.m_collection[posCompare];

		// shift alignment
		alignCompare.Shift( (int)shiftCompare );

		if (!alignThis.Equals(alignCompare))
			return false;

		posCompare++;
	}

	return true;
}

void AlignmentPhrase::Merge(const AlignmentPhrase &newAlignment, size_t shift, size_t startPos)
{
	for (size_t pos = 0 ; pos < newAlignment.GetSize() ; ++pos)
	{
		size_t insertPos = pos + startPos;

		// shift alignment
		AlignmentElement alignElement = newAlignment.m_collection[pos];
		alignElement.Shift( (int)shift );
		
		if (insertPos >= GetSize())
		{ // probably doing target. append alignment to end
			assert(insertPos == GetSize());
			m_collection.push_back(alignElement);
		}
		else
		{ // should really merge, rather than replace element
			m_collection[insertPos] = alignElement;
		}
	}
}

bool AlignmentPhrase::IsCompletable(size_t decodeStepId
																		, const WordsBitmap &thisCompleted
																		, const WordsBitmap &otherCompleted) const
{
	for (size_t pos = 0 ; pos < thisCompleted.GetSize() ; ++pos)
	{
		if (!thisCompleted.GetValue(decodeStepId, pos))
		{ // not yet decoded, see if can be aligned
			const AlignmentElement &element = m_collection[pos];
			bool completable = false;

			if (element.IsEmpty())
			{ // nothing for this element
				continue;
			}

			AlignmentElement::const_iterator iter;
			for (iter = element.begin() ; iter != element.end() ; ++iter)
			{
				size_t alignPos = *iter;
				bool completed = otherCompleted.GetValue(decodeStepId, alignPos);
				if (!completed)
				{ // not already taken, the element can possibly be aligned in the future
					completable = true;
					break;
				}
			}

			if (!completable) // 1 element can't be aligned
				return false;
		}
	}

	// all elements can be aligned
	return true;
}

std::ostream& operator<<(std::ostream& out, const AlignmentPhrase &alignmentPhrase)
{
	for (size_t pos = 0 ; pos < alignmentPhrase.GetSize() ; ++pos)
	{
		const AlignmentElement &alignElement = alignmentPhrase.m_collection[pos];
		out << "[" << alignElement << "] ";
	}
	return out;
}


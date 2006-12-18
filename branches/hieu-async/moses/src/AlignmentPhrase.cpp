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

#include <algorithm>
#include "AlignmentPhrase.h"
#include "WordsRange.h"

using namespace std;

AlignmentElement::AlignmentElement(const std::vector<size_t> &copy)
{
	std::copy(copy.begin(), copy.end(), back_insert_iterator< AlignmentElement > (*this));
}

bool AlignmentPhrase::IsCompatible(const AlignmentPhrase &compare, size_t startPosCompare) const 
{
	const size_t endPos = std::min(size() , startPosCompare + compare.size());
	size_t posCompare = 0;
	for (size_t posThis = startPosCompare ; posThis < endPos ; ++posThis)
	{
		const AlignmentElement &alignThis = (*this)[posThis];
		AlignmentElement alignCompare = compare[posCompare];

		// shift alignment
		for (size_t index = 0 ; index < alignCompare.size() ; ++index)
			alignCompare[index] += startPosCompare;

		if (!alignThis.Equals(alignCompare))
			return false;

		posCompare++;
	}

	return true;
}

void AlignmentPhrase::Merge(const AlignmentPhrase &newAlignment, const WordsRange &newAlignmentRange)
{
	size_t index = 0;
	for (size_t pos = newAlignmentRange.GetStartPos() ; pos <= newAlignmentRange.GetEndPos() ; ++pos)
	{
		if (pos >= this->size())
		{
			// shift alignment
			AlignmentElement alignVec = newAlignment[index++];
			for (size_t index = 0 ; index < alignVec.size() ; ++index)
				alignVec[index] += newAlignmentRange.GetStartPos();

			push_back(alignVec);
		}
	}
}

std::ostream& operator<<(std::ostream& out, const AlignmentPhrase &alignmentPhrase)
{
	for (size_t pos = 0 ; pos < alignmentPhrase.size() ; ++pos)
	{
		const AlignmentElement &alignment = alignmentPhrase[pos];
		out << "[";
		for (size_t index = 0 ; index < alignment.size() ; ++index)
			out << alignment[index];
		out << "] ";
	}
	return out;
}
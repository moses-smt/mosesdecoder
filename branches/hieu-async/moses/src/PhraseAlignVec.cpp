// $Id: PhraseAlignVec.cpp 988 2006-11-21 19:35:37Z hieuhoang1972 $
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

#include "PhraseAlignVec.h"
#include "WordsRange.h"

using namespace std;

bool PhraseAlignVec::IsCompatible(const PhraseAlignVec &compare, size_t startPosCompare) const 
{
	const size_t endPos = std::min(size() , startPosCompare + compare.size());
	size_t posCompare = 0;
	for (size_t posThis = startPosCompare ; posThis < endPos ; ++posThis)
	{
		if ((*this)[posThis] != compare[posCompare])
			return false;

		posCompare++;
	}

	return true;
}

void PhraseAlignVec::Merge(const PhraseAlignVec &newAlignment, const WordsRange &newAlignmentRange)
{
	size_t index = 0;
	for (size_t pos = newAlignmentRange.GetStartPos() ; pos <= newAlignmentRange.GetEndPos() ; ++pos)
	{
		if (pos >= this->size())
		{
			AlignVec alignVec = newAlignment[index++];
			for (size_t index = 0 ; index < alignVec.size() ; ++index)
				alignVec[index] += newAlignmentRange.GetStartPos();

			push_back(alignVec);
		}
	}
}

std::ostream& operator<<(std::ostream& out, const PhraseAlignVec &phraseAlignment)
{
	for (size_t pos = 0 ; pos < phraseAlignment.size() ; ++pos)
	{
		const AlignVec &alignment = phraseAlignment[pos];
		out << "[";
		for (size_t index = 0 ; index < alignment.size() ; ++index)
			out << alignment[index];
		out << "] ";
	}
	return out;
}
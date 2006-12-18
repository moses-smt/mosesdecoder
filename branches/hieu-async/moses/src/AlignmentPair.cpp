// $Id: AlignmentPair.cpp 988 2006-11-21 19:35:37Z hieuhoang1972 $
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

#include "AlignmentPair.h"
#include "AlignmentPhrase.h"

using namespace std;

AlignmentPairInserter AlignmentPair::GetInserter(FactorDirection direction)
{
	return (direction == Input) ? back_insert_iterator<AlignmentPhrase>(m_sourceAlign)
															: back_insert_iterator<AlignmentPhrase>(m_targetAlign);
}

TO_STRING_BODY(AlignmentPair);

void OutputPhraseAlignVec(std::ostream &out, const AlignmentPhrase &phraseAlignVec)
{
	
	for (size_t pos = 0 ; pos < phraseAlignVec.size() ; ++pos)
	{
		const AlignmentElement &alignVec = phraseAlignVec[pos];
		if (alignVec.size() > 0)
		{
			out << "[" << alignVec[0];
			for (size_t index = 1 ; index < alignVec.size() ; ++index)
			{
				out << "," << alignVec[index];
			}
			out << "] ";
		}
	}
	
}

void AlignmentPair::SetAlignment()
{
	AlignmentElement alignment;
	alignment.push_back(0);
	m_sourceAlign.push_back(alignment);
	m_targetAlign.push_back(alignment);
}

std::ostream& operator<<(std::ostream &out, const AlignmentPair &alignmentPair)
{
	OutputPhraseAlignVec(out, alignmentPair.m_sourceAlign);
	out << " to ";
	OutputPhraseAlignVec(out, alignmentPair.m_targetAlign);
	return out;
}
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

AlignmentPhraseInserter AlignmentPair::GetInserter(FactorDirection direction)
{
	return (direction == Input) ? back_insert_iterator<AlignmentPhrase>(m_sourceAlign)
															: back_insert_iterator<AlignmentPhrase>(m_targetAlign);
}

void AlignmentPair::SetIdentityAlignment()
{
	AlignmentElement alignment;
	alignment.SetIdentityAlignment();
	
	m_sourceAlign.push_back(alignment);
	m_targetAlign.push_back(alignment);
}

bool AlignmentPair::IsCompletable() const
{
	// TODO
	return true;
}

void AlignmentPair::Merge(const AlignmentPair &newAlignment, const WordsRange &sourceRange, const WordsRange &targetRange)
{
	m_sourceAlign.Merge(newAlignment.m_sourceAlign, sourceRange);
	m_targetAlign.Merge(newAlignment.m_targetAlign, sourceRange);
}

TO_STRING_BODY(AlignmentPair);

std::ostream& operator<<(std::ostream &out, const AlignmentPair &alignmentPair)
{
	out << alignmentPair.m_sourceAlign
			<< " to "
			<< alignmentPair.m_targetAlign;
	return out;
}


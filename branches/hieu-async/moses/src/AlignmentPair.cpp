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

#include "AlignmentPair.h"
#include "AlignmentPhrase.h"
#include "WordsRange.h"

using namespace std;

AlignmentPhraseInserter AlignmentPair::GetInserter(FactorDirection direction)
{
	return (direction == Input) ? back_insert_iterator<AlignmentPhrase::CollectionType>(m_sourceAlign.GetVector())
															: back_insert_iterator<AlignmentPhrase::CollectionType>(m_targetAlign.GetVector());
}

void AlignmentPair::SetIdentityAlignment()
{
	AlignmentElement alignment;
	alignment.SetIdentityAlignment();
	
	m_sourceAlign.Add(alignment);
	m_targetAlign.Add(alignment);
}

bool AlignmentPair::IsCompletable(size_t decodeStepId
																	, const WordsBitmap &sourceCompleted
																	, const WordsBitmap &targetCompleted) const
{
	if (!m_sourceAlign.IsCompletable(decodeStepId, sourceCompleted, targetCompleted))
		return false;

	return m_targetAlign.IsCompletable(decodeStepId, targetCompleted, sourceCompleted);
}

bool AlignmentPair::IsCompatible(const AlignmentPair &compare
																, size_t sourceStart
																, size_t targetStart) const
{
	// source
	bool ret = GetAlignmentPhrase(Input).IsCompatible(
							compare.GetAlignmentPhrase(Input)
							, sourceStart
							, targetStart);

	if (!ret)
		return false;

	// target
	return GetAlignmentPhrase(Output).IsCompatible(
							compare.GetAlignmentPhrase(Output)
							, targetStart
							, sourceStart);
}

void AlignmentPair::Add(const AlignmentPair &newAlignment
												, const WordsRange &sourceRange
												, const WordsRange &targetRange)
{
	m_sourceAlign.Add(newAlignment.m_sourceAlign
										, targetRange.GetStartPos()
										, sourceRange.GetStartPos());	
	m_targetAlign.Add(newAlignment.m_targetAlign
											, sourceRange.GetStartPos()
											, targetRange.GetStartPos());
}

void AlignmentPair::Merge(const AlignmentPair &newAlignment, const WordsRange &sourceRange, const WordsRange &targetRange)
{
	m_sourceAlign.Merge(newAlignment.m_sourceAlign
										, targetRange.GetStartPos()
										, sourceRange.GetStartPos());	
	m_targetAlign.Merge(newAlignment.m_targetAlign
											, sourceRange.GetStartPos()
											, targetRange.GetStartPos());
}

TO_STRING_BODY(AlignmentPair);

std::ostream& operator<<(std::ostream &out, const AlignmentPair &alignmentPair)
{
	out << alignmentPair.m_sourceAlign
			<< " to "
			<< alignmentPair.m_targetAlign;
	return out;
}


// $Id: AlignmentPhrase.cpp 552 2009-01-09 14:05:34Z hieu $
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

AlignmentPhrase::AlignmentPhrase(const AlignmentPhrase &copy)
: m_collection(copy.m_collection.size())
{
	for (size_t pos = 0 ; pos < copy.m_collection.size() ; ++pos)
	{
		if (copy.Exists(pos))
			m_collection[pos] = new AlignmentElement(copy.GetElement(pos));
		else
			m_collection[pos] = NULL;
	}
}

AlignmentPhrase::AlignmentPhrase(size_t size)
	:m_collection(size)
{
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		m_collection[pos] = NULL;
	}
}

AlignmentPhrase::~AlignmentPhrase()
{
	RemoveAllInColl(m_collection);
}

bool AlignmentPhrase::IsCompatible(const AlignmentPhrase &compare
																	 , size_t mergePosStart
																	 , size_t shiftPos
																	 , bool allowSourceNullAlign) const 
{
	const size_t compareSize = min(GetSize() - mergePosStart	, compare.GetSize());

	size_t posThis = mergePosStart;
	for (size_t posCompare = 0 ; posCompare < compareSize ; ++posCompare)
	{
		if (!Exists(posThis))
			continue;
		assert(posThis < GetSize());
		
		const AlignmentElement &alignThis = GetElement(posThis);
		if (allowSourceNullAlign && alignThis.GetSize() == 0)
		{
			posThis++;
			continue;
		}

		AlignmentElement alignCompare = compare.GetElement(posCompare);

		// shift alignment
		alignCompare.Shift( (int)shiftPos);

    if (!alignThis.Intersect(alignCompare))
			return false;

		posThis++;
	}

	return true;
}

void AlignmentPhrase::Add(const AlignmentPhrase &newAlignment, size_t shift, size_t startPos)
{
	size_t insertPos = startPos;
	for (size_t pos = 0 ; pos < newAlignment.GetSize() ; ++pos)
	{
		// shift alignment
		AlignmentElement alignElement = newAlignment.GetElement(pos);
		alignElement.Shift( (int)shift );
		
		if (insertPos >= GetSize())
		{ // probably doing target. append alignment to end
			assert(insertPos == GetSize());
			Add(alignElement);
		}
		else
		{
			if (Exists(insertPos))
			{ // presumably just a placeholder
				assert(m_collection[insertPos]->GetSize() == 0);
				m_collection[insertPos]->Replace(alignElement);
			}
			else
				m_collection[insertPos] = new AlignmentElement(alignElement);
		}

		insertPos++;
	}
}

void AlignmentPhrase::Merge(const AlignmentPhrase &newAlignment, size_t shift, size_t startPos)
{
	assert(startPos < GetSize());
	
	size_t insertPos = startPos;
	for (size_t pos = 0 ; pos < newAlignment.GetSize() ; ++pos)
	{
		// shift alignment
		AlignmentElement alignElement = newAlignment.GetElement(pos);
		alignElement.Shift( (int)shift );
		
		// merge elements to only contain co-joined elements
		GetElement(insertPos).SetIntersect(alignElement);

		insertPos++;
	}
}

bool AlignmentPhrase::IsCompletable(size_t decodeStepId
																		, const WordsBitmap &thisCompleted
																		, const WordsBitmap &otherCompleted) const
{
	for (size_t pos = 0 ; pos < thisCompleted.GetSize() ; ++pos)
	{
		if (!thisCompleted.GetValue(pos))
		{ // not yet decoded, see if can be aligned
			bool completable = false;

			if (!Exists(pos))
			{ // not yet aligned
				continue;
			}

			const AlignmentElement &element = GetElement(pos);
			AlignmentElement::const_iterator iter;
			for (iter = element.begin() ; iter != element.end() ; ++iter)
			{
				size_t alignPos = *iter;
				bool completed = otherCompleted.GetValue(alignPos);
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

void AlignmentPhrase::AddUniformAlignmentElement(std::list<size_t> &uniformAlignmentTarget)
{
	list<size_t>::iterator iter;
	for (iter = uniformAlignmentTarget.begin() ; iter != uniformAlignmentTarget.end() ; ++iter)
	{
		for (size_t pos = 0 ; pos < GetSize() ; ++pos)
		{
			AlignmentElement &alignElement = GetElement(pos);
			alignElement.Add(*iter);
		}
	}
}

std::ostream& operator<<(std::ostream& out, const AlignmentPhrase &alignmentPhrase)
{
	for (size_t pos = 0 ; pos < alignmentPhrase.GetSize() ; ++pos)
	{
		if (alignmentPhrase.Exists(pos))
		{
			const AlignmentElement &alignElement = alignmentPhrase.GetElement(pos);
			out << alignElement << " ";
		}
		else
			out << "() ";
	}
	return out;
}

TO_STRING_BODY(AlignmentPhrase);


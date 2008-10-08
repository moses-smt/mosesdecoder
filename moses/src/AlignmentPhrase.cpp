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

#include "AlignmentPhrase.h"
#include "WordsRange.h"
#include "WordsBitmap.h"
#include "UserMessage.h"

using namespace std;

namespace Moses
{

void EmptyAlignment(string &Align, size_t Size)
{
	Align = " ";
	for (size_t pos = 0 ; pos < Size ; ++pos)
		Align += "() ";
}

void UniformAlignment(string &Align, size_t fSize, size_t eSize)
{
	std::stringstream AlignStream;
	for (size_t fpos = 0 ; fpos < fSize ; ++fpos){
		AlignStream << "(";
		for (size_t epos = 0 ; epos < eSize ; ++epos){
			if (epos) AlignStream << ",";
			AlignStream << epos;
		}
		AlignStream << ") ";
	}
	Align = AlignStream.str();
}

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

AlignmentPhrase& AlignmentPhrase::operator=(const AlignmentPhrase &copy)
{
	m_collection.resize(copy.GetSize());
	//	m_collection=AlignmentPhrase(copy.GetSize());
	for (size_t pos = 0 ; pos < copy.GetSize() ; ++pos)
	{
		if (copy.Exists(pos))
			m_collection[pos] = new AlignmentElement(copy.GetElement(pos));
		else
			m_collection[pos] = NULL;
	}
	return *this;
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

bool AlignmentPhrase::IsCompatible(const AlignmentPhrase &compare, size_t mergePosStart, size_t shiftPos) const 
{
	const size_t compareSize = min(GetSize() - mergePosStart	, compare.GetSize());

	size_t posThis = mergePosStart;
	for (size_t posCompare = 0 ; posCompare < compareSize ; ++posCompare)
	{
		if (!Exists(posThis))
			continue;
		assert(posThis < GetSize());
		
		const AlignmentElement &alignThis = GetElement(posThis);
		AlignmentElement alignCompare = compare.GetElement(posCompare);

		// shift alignment
		alignCompare.Shift( (int)shiftPos);

		if (!alignThis.Equals(alignCompare))
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
			{ // add
				m_collection[insertPos]->SetIntersect(alignElement);
			}
			else
				m_collection[insertPos] = new AlignmentElement(alignElement);
		}

		insertPos++;
	}
}

void AlignmentPhrase::Shift(size_t shift)
{
	for (size_t pos = 0 ; pos < GetSize() ; ++pos)
	{
		// shift alignment
		GetElement(pos).Shift( (int)shift );
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
			if (pos) out << " ";
			const AlignmentElement &alignElement = alignmentPhrase.GetElement(pos);
			out << alignElement;
		}
		else{
			stringstream strme;
			strme << "No alignment at position " << pos;
			UserMessage::Add(strme.str());
			abort();
		}
	}
	return out;
}

void AlignmentPhrase::print(std::ostream& out, size_t offset) const
{
	
	for (size_t pos = 0 ; pos < GetSize() ; ++pos)
	{
		if (Exists(pos))
		{
			if (pos) out << " ";
			out << pos+offset << "=";
			const AlignmentElement &alignElement = GetElement(pos);
			out << alignElement;
		}
		else{
			stringstream strme;
			strme << "No alignment at position " << pos;
			UserMessage::Add(strme.str());
			abort();
//			out << pos+offset << "=";
		}
	}
}

TO_STRING_BODY(AlignmentPhrase);


}


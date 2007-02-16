// $Id: AlignmentElement.h 988 2006-11-21 19:35:37Z hieuhoang1972 $
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

#pragma once

#include <iostream>
#include <set>
#include <vector>

//! set of alignments of 1 word
class AlignmentElement
{
	friend std::ostream& operator<<(std::ostream& out, const AlignmentElement &alignElement);

protected:
	typedef std::set<size_t> ContainerType;
	ContainerType m_collection;
public:
	typedef ContainerType::iterator iterator;
	typedef ContainerType::const_iterator const_iterator;
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }

	AlignmentElement()
	{}

	//! inital constructor from parsed info from phrase table
	AlignmentElement(const std::vector<size_t> &alignInfo); 
	
	//! number of words this element aligns to
	size_t GetSize() const
	{ 
		return m_collection.size();
	}

	bool IsEmpty() const
	{
		return m_collection.empty();
	}
	
	//! return internal collection of elements
	const ContainerType &GetCollection() const
	{
		return m_collection;
	}

	/** compare all alignments for this word. 
		*	Return true iff both words are aligned to the same words
	*/
	bool Equals(const AlignmentElement &compare) const
	{
		return m_collection == compare.GetCollection();
	}
	
		/** used by the unknown word handler.
		* Set alignment to 0
		*/
	void SetIdentityAlignment()
	{
		m_collection.insert(0);
	}

	// shift alignment so that it is comparitable to another alignment. 
	void Shift(int shift);
};

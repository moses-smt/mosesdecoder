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

#pragma once

#include <limits>
#include <vector>
#include <iostream>
#include <cstring>
#include <cmath>
#include "TypeDef.h"
#include "WordsRange.h"

/** vector of boolean used to represent whether a word has been translated or not
*/
class WordsBitmap 
{
	friend std::ostream& operator<<(std::ostream& out, const WordsBitmap& wordsBitmap);
protected:
	const size_t m_size; /**< number of words in sentence */
	bool	*m_bitmap;	/**< ticks of words that have been done */

	WordsBitmap(); // not implemented

	//! set all elements to false
	void Initialize()
	{
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			m_bitmap[pos] = false;
		}
	}

public:
	//! create WordsBitmap of length size and initialise
	WordsBitmap(size_t size)
		:m_size	(size)
	{
		m_bitmap = (bool*) malloc(sizeof(bool) * size);
		Initialize();
	}
	//! deep copy
	WordsBitmap(const WordsBitmap &copy)
		:m_size		(copy.m_size)
	{
		m_bitmap = (bool*) malloc(sizeof(bool) * m_size);
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			m_bitmap[pos] = copy.GetValue(pos);
		}
	}
	~WordsBitmap()
	{
		free(m_bitmap);
	}
	//! count of words translated
	size_t GetNumWordsCovered() const
	{
		size_t count = 0;
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			if (m_bitmap[pos])
				count++;
		}
		return count;
	}

	//! position of 1st word not yet translated, or NOT_FOUND if everything already translated
	size_t GetFirstGapPos() const
	{
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			if (!m_bitmap[pos])
			{
				return pos;
			}
		}
		// no starting pos
		return NOT_FOUND;
	}

	//! position of last translated word
	size_t GetLastPos() const
	{
		for (int pos = (int) m_size - 1 ; pos >= 0 ; pos--)
		{
			if (m_bitmap[pos])
			{
				return pos;
			}
		}
		// no starting pos
		return NOT_FOUND;
	}

	//! whether a word has been translated at a particular position
	bool GetValue(size_t pos) const
	{
		return m_bitmap[pos];
	}
	//! set value at a particular position
	void SetValue( size_t pos, bool value )
	{
		m_bitmap[pos] = value;
	}
	//! set value between 2 positions, inclusive
	void SetValue( size_t startPos, size_t endPos, bool value )
	{
		for(size_t pos = startPos ; pos <= endPos ; pos++) 
		{
			m_bitmap[pos] = value;
		}
	}
	//! whether every word has been translated
	bool IsComplete() const
	{
		return GetSize() == GetNumWordsCovered();
	}
	//! whether the wordrange overlaps with any translated word in this bitmap
	bool Overlap(const WordsRange &compare) const
	{
		for (size_t pos = compare.GetStartPos() ; pos <= compare.GetEndPos() ; pos++)
		{
			if (m_bitmap[pos])
				return true;
		}
		return false;
	}
	//! number of elements
	size_t GetSize() const
	{
		return m_size;
	}
	/** represent this bitmap as 1 or more vector of integers.
		* Used for exact matching of source words translated in hypothesis recombination
		*/
	std::vector<size_t> GetCompressedRepresentation() const;
	
	//! transitive comparison of WordsBitmap
	inline int Compare (const WordsBitmap &compare) const
	{
		// -1 = less than
		// +1 = more than
		// 0	= same

		size_t thisSize = GetSize()
					,compareSize = compare.GetSize();

		if (thisSize != compareSize)
		{
			return (thisSize < compareSize) ? -1 : 1;
		}
    		return std::memcmp(m_bitmap, compare.m_bitmap, thisSize);
	}

	inline size_t GetEdgeToTheLeftOf(size_t l) const
	{
		if (l == 0) return l;
		--l;
		while (!m_bitmap[l] && l) { --l; }
		return l;
	}

	inline size_t GetEdgeToTheRightOf(size_t r) const
	{
		if (r+1 == m_size) return r;
		++r;
		while (!m_bitmap[r] && r < m_size) { ++r; }
		return r;
	}


	//! TODO - ??? no idea
	int GetFutureCosts(int lastPos) const ;

	TO_STRING();
};

// friend 
inline std::ostream& operator<<(std::ostream& out, const WordsBitmap& wordsBitmap)
{
	for (size_t i = 0 ; i < wordsBitmap.m_size ; i++)
	{
		out << (wordsBitmap.GetValue(i) ? 1 : 0);
	}
	return out;
}

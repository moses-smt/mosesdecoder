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
#include "TypeDef.h"
#include "WordsRange.h"

class WordsBitmap 
{
	friend std::ostream& operator<<(std::ostream& out, const WordsBitmap& wordsBitmap);
protected:
	const size_t m_size;
	bool	*m_bitmap;
	// ticks of words that have been done;

	WordsBitmap(); // not implemented

	void Initialize()
	{
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			m_bitmap[pos] = false;
		}
	}

public:
	WordsBitmap(size_t size)
		:m_size	(size)
	{
		m_bitmap = (bool*) malloc(sizeof(bool) * size);
		Initialize();
	}
	
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

	size_t GetWordsCount() const
	{
		size_t count = 0;
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			if (m_bitmap[pos])
				count++;
		}
		return count;
	}

	size_t GetFirstGapPos() const
	{
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			if (m_bitmap[pos] == 0)
			{
				return pos;
			}
		}
		// no starting pos
		return NOT_FOUND;
	}
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
	bool GetValue(size_t pos) const
	{
		return m_bitmap[pos];
	}
	void SetValue( size_t pos, bool value )
	{
		m_bitmap[pos] = value;
	}
	void SetValue( size_t startPos, size_t endPos, bool value )
	{
		for(size_t pos = startPos ; pos <= endPos ; pos++) 
		{
			m_bitmap[pos] = value;
		}
	} 
	bool IsComplete() const
	{
		return GetSize() == GetWordsCount();
	}

	bool Overlap(const WordsRange &compare) const
	{
		for (size_t pos = compare.GetStartPos() ; pos <= compare.GetEndPos() ; pos++)
		{
			if (m_bitmap[pos])
				return true;
		}
		return false;
	}

	size_t GetSize() const
	{
		return m_size;
	}
	
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

	TO_STRING;
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


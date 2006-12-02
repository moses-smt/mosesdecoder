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
#include <map>
#include <vector>
#include <iostream>
#include <cstring>
#include <cmath>
#include "TypeDef.h"
#include "WordsRange.h"

class DecodeStep;

/** vector of boolean used to represent whether a word has been translated or not
*/
class WordsBitmap 
{
	friend std::ostream& operator<<(std::ostream& out, const WordsBitmap& wordsBitmap);
protected:
	typedef std::map<const DecodeStep*, bool	*> BitmapType;
	const size_t m_size; /**< number of words in sentence */
	BitmapType m_bitmap;	/**< ticks of words that have been done */

	WordsBitmap(); // not implemented

	//! set all elements to false
	void Initialize();

	bool *GetBitmap(const DecodeStep *decoderStep) const
	{
		BitmapType::const_iterator iter = m_bitmap.find(decoderStep);
		assert(iter != m_bitmap.end());
		bool *bitmap = iter->second;
		return bitmap;
	}

public:
	//! create WordsBitmap of length size and initialise
	WordsBitmap(const std::list < DecodeStep* > &decodeStepList, size_t size);

	//! deep copy
	WordsBitmap(const WordsBitmap &copy);

	~WordsBitmap();

	//! count of words translated
	size_t GetNumWordsCovered(const DecodeStep *decoderStep) const
	{
		bool *bitmap = GetBitmap(decoderStep);
		size_t count = 0;
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			if (bitmap[pos])
				count++;
		}
		return count;
	}

	//! position of 1st word not yet translated, or NOT_FOUND if everything already translated
	size_t GetFirstGapPos(const DecodeStep *decoderStep) const
	{
		bool *bitmap = GetBitmap(decoderStep);
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			if (!bitmap[pos])
			{
				return pos;
			}
		}
		// no starting pos
		return NOT_FOUND;
	}

	//! position of last translated word
	size_t GetLastPos(const DecodeStep *decoderStep) const
	{
		bool *bitmap = GetBitmap(decoderStep);
		for (int pos = (int) m_size - 1 ; pos >= 0 ; pos--)
		{
			if (bitmap[pos])
			{
				return pos;
			}
		}
		// no starting pos
		return NOT_FOUND;
	}

	//! whether a word has been translated at a particular position
	bool GetValue(const DecodeStep *decoderStep, size_t pos) const
	{
		bool *bitmap = GetBitmap(decoderStep);
		return bitmap[pos];
	}
	//! set value at a particular position
	void SetValue(const DecodeStep *decoderStep, size_t pos, bool value )
	{
		bool *bitmap = GetBitmap(decoderStep);
		bitmap[pos] = value;
	}
	//! set value between 2 positions, inclusive
	void SetValue(const DecodeStep *decoderStep, size_t startPos, size_t endPos, bool value )
	{
		bool *bitmap = GetBitmap(decoderStep);
		for(size_t pos = startPos ; pos <= endPos ; pos++) 
		{
			bitmap[pos] = value;
		}
	}
	//! whether every word has been translated
	bool IsComplete(const DecodeStep *decoderStep) const
	{
		return GetSize() == GetNumWordsCovered(decoderStep);
	}
	//! whether the wordrange overlaps with any translated word in this bitmap
	bool Overlap(const DecodeStep *decoderStep, const WordsRange &compare) const
	{
		bool *bitmap = GetBitmap(decoderStep);

		for (size_t pos = compare.GetStartPos() ; pos <= compare.GetEndPos() ; pos++)
		{
			if (bitmap[pos])
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
	std::vector<size_t> GetCompressedRepresentation(const DecodeStep *decoderStep) const;
	
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

		BitmapType::const_iterator iter;
		for (iter = m_bitmap.begin() ; iter != m_bitmap.end() ; ++iter)
		{
			const DecodeStep *decodeStep =iter->first;
			const bool *bitmap				= iter->second
								,*compareBitmap	= compare.GetBitmap(decodeStep);

	    int ret = std::memcmp(bitmap, compareBitmap, thisSize);
			if (ret != 0)
				return ret>0 ? 1 : -1;
		}

		return 0;
	}

	//! TODO - ??? no idea
	int GetFutureCosts(const DecodeStep *decoderStep, int lastPos) const ;

	TO_STRING();
};

// friend 
inline std::ostream& operator<<(std::ostream& out, const WordsBitmap& wordsBitmap)
{
	WordsBitmap::BitmapType::const_iterator iter;
	for (iter = wordsBitmap.m_bitmap.begin() ; iter != wordsBitmap.m_bitmap.end() ; ++iter)
	{
		const DecodeStep *decodeStep =iter->first;
		for (size_t i = 0 ; i < wordsBitmap.m_size ; i++)
		{
			out << (wordsBitmap.GetValue(decodeStep, i) ? 1 : 0);
		}
	}
	return out;
}

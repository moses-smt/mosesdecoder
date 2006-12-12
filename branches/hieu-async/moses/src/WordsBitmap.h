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
class FactorMask;

/** vector of boolean used to represent whether a word has been translated or not
*/
class WordsBitmap 
{
	friend std::ostream& operator<<(std::ostream& out, const WordsBitmap& wordsBitmap);
protected:
	typedef std::vector<bool*> BitmapType;
	const size_t m_size; /**< number of words in sentence */
	BitmapType m_bitmap;	/**< ticks of words that have been done */

	WordsBitmap(); // not implemented

	//! set all elements to false
	void Initialize();

	//! most optimistic distortion cost
	int GetFutureDistortionScore(size_t decodeStepId, int lastPos) const;

	/** represent this bitmap as 1 or more vector of integers.
	* Used for exact matching of source words translated in hypothesis recombination
	*/
	std::vector<size_t> GetCompressedRepresentation(size_t decodeStepId) const;

public:
	//! create WordsBitmap of length size and initialise
	WordsBitmap(size_t size);

	//! deep copy
	WordsBitmap(const WordsBitmap &copy);

	~WordsBitmap();

	//! count of words translated
	size_t GetNumWordsCovered(size_t decodeStepId) const
	{
		bool *bitmap = m_bitmap[decodeStepId];
		size_t count = 0;
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			if (bitmap[pos])
				count++;
		}
		return count;
	}

	//! position of 1st word not yet translated, or NOT_FOUND if everything already translated
	size_t GetFirstGapPos(size_t decodeStepId) const
	{
		bool *bitmap = m_bitmap[decodeStepId];
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

	bool GetValue(size_t decodeStepId, size_t pos) const
	{
		bool *bitmap = m_bitmap[decodeStepId];
		return bitmap[pos];
	}

	//! set value at a particular position
	void SetValue(size_t decodeStepId, size_t pos, bool value )
	{
		bool *bitmap = m_bitmap[decodeStepId];
		bitmap[pos] = value;
	}

	//! set value between 2 positions, inclusive
	void SetValue(size_t decodeStepId, size_t startPos, size_t endPos, bool value )
	{
		bool *bitmap = m_bitmap[decodeStepId];
		for(size_t pos = startPos ; pos <= endPos ; pos++) 
		{
			bitmap[pos] = value;
		}
	}

	//! set value between 2 positions, inclusive
	void SetValue(const WordsRange &wordsRange, bool value )
	{
		SetValue(wordsRange.GetDecodeStepId()
						, wordsRange.GetStartPos()
						, wordsRange.GetEndPos()
						, value);
	}

	//! whether every word has been translated for a particular factor type
	bool IsComplete(FactorType factorType) const;
	bool IsComplete(const FactorMask &factorMask) const;
	//! whether every word has been translated
	bool IsComplete(const DecodeStep &decodeStep) const;

	/** check the previous decode step. ok if this step only covers span that previous
	 *	step already covers
	 */
	bool IsHierarchy(size_t decodeStepId, size_t startPos, size_t endPos) const;

	//! whether the wordrange overlaps with any translated word in this bitmap
	bool Overlap(const WordsRange &compare) const
	{
		size_t decodeStepId = compare.GetDecodeStepId();
		bool *bitmap = m_bitmap[decodeStepId];

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

		for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
		{
			const bool *bitmap				= m_bitmap[decodeStepId]
								,*compareBitmap	= compare.m_bitmap[decodeStepId];

	    int ret = std::memcmp(bitmap, compareBitmap, thisSize);
			if (ret != 0)
				return ret>0 ? 1 : -1;
		}
		return 0;
	}

	//! most optmistic distortion cost to completely translate source
	int GetFutureDistortionScore(int lastPos) const ;

	//! calc which stack that a hypothesis with this source bitmap should be in 
	size_t GetStackIndex() const;

	TO_STRING();
};

// friend 
inline std::ostream& operator<<(std::ostream& out, const WordsBitmap& wordsBitmap)
{
	for (size_t decodeStepId = 0 ; decodeStepId < wordsBitmap.m_bitmap.size() ; ++decodeStepId)
	{
		for (size_t i = 0 ; i < wordsBitmap.m_size ; i++)
		{
			out << (wordsBitmap.GetValue(decodeStepId, i) ? 1 : 0);
		}
	}
	return out;
}

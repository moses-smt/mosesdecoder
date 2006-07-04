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

#include "TypeDef.h"

class WordsRange
{ // slimmed down, contiguous version of WordsBitmap
	size_t m_startPos, m_endPos;
public:
	inline size_t GetStartPos() const
	{
		return m_startPos;
	}
	inline size_t GetEndPos() const
	{
		return m_endPos;
	}
	inline WordsRange(size_t startPos, size_t endPos)
	{
		m_startPos	= startPos;
		m_endPos		= endPos;
	}

	inline int CalcDistortion(const WordsRange &prevRange) const
	{
		int dist = (int)prevRange.GetEndPos() - (int)GetStartPos() + 1 ;
		return abs(dist);
	}

	inline size_t GetWordsCount() const
	{
		return (m_startPos == NOT_FOUND) ? 0 : m_endPos - m_startPos + 1;
	}

};


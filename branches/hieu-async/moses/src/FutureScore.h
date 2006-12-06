// $Id: TranslationOptionCollection.h 1034 2006-12-01 22:10:23Z hieuhoang1972 $

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

#include <map>
#include "SquareMatrix.h"

class DecodeStep;
class WordsBitmap;

class FutureScore
{
protected:
	typedef std::map<const DecodeStep*, SquareMatrix*> FutureScoreType;
	const size_t m_size;
	FutureScoreType	m_futureScore; /*< matrix of future costs for contiguous parts (span) of the input */

	FutureScore(); // not implemented
	FutureScore(const FutureScore &copy); // not implemented

	const SquareMatrix *GetSquareMatrix(const DecodeStep *decodeStep) const
	{
		FutureScoreType::const_iterator iter = m_futureScore.find(decodeStep);
		assert(iter != m_futureScore.end());
		SquareMatrix *sqMatrix = iter->second;
		return sqMatrix;
	}

public:
	FutureScore(size_t size)
	:m_size(size)
	{}
	~FutureScore();

	inline size_t GetSize() const
	{
		return m_size;
	}

	void AddDecodeStep(const DecodeStep *decodeStep);
	//! get a score for a range, for a particular step
	inline float GetScore(const DecodeStep *decodeStep, size_t row, size_t col) const
	{
		assert(row <= col);
		const SquareMatrix *sqMatrix = GetSquareMatrix(decodeStep);
		return sqMatrix->GetScore(row, col);
	}
	//! set a score for a range, for a particular step
	inline void SetScore(const DecodeStep *decodeStep, size_t row, size_t col, float value)
	{
		assert(row <= col);
		m_futureScore[decodeStep]->SetScore(row, col, value);
	}

	//! calc of future in all steps, for all untranslated contiguous source phrase
	float GetFutureScore(const WordsBitmap &sourceCompleted) const;

};


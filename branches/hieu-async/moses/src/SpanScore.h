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

#include <map>
#include <list>
#include "SquareMatrix.h"

class DecodeStep;
class WordsBitmap;

class SpanScore
{
protected:
	typedef std::vector< SquareMatrix<float> *> SpanScoreType;
	const size_t m_size;
	SpanScoreType	m_matrices; /*< matrix of future costs for contiguous parts (span) of the input */

	SpanScore(); // not implemented
	SpanScore(const SpanScore &copy); // not implemented

	const SquareMatrix<float> *GetSquareMatrix(size_t decodeStepId) const
	{
		assert(decodeStepId < m_matrices.size());
		return m_matrices[decodeStepId];
	}

public:
	SpanScore(size_t size)
	:m_size(size)
	{}
	~SpanScore();

	//! resize number of square matrices and initiliase all elements to -inf
	void Initialize(const std::vector<const DecodeStep*> &decodeStepList);

	inline size_t GetSize() const
	{
		return m_size;
	}

	//! get a score for a range, for a particular step
	inline float GetScore(size_t decodeStepId, size_t startPos, size_t endPos) const
	{
		assert(startPos <= endPos);
		const SquareMatrix<float> *sqMatrix = GetSquareMatrix(decodeStepId);
		return sqMatrix->GetScore(startPos, endPos);
	}
	//! set a score for a range, for a particular step
	inline void SetScore(size_t decodeStepId, size_t row, size_t col, float value)
	{
		assert(row <= col);
		m_matrices[decodeStepId]->SetScore(row, col, value);
	}

	//! calc of future in all steps, for all untranslated contiguous source phrase
	float GetFutureScore(const WordsBitmap &sourceCompleted) const;

	/** Fill all the cells in the strictly upper triangle
		* there is no way to modify the diagonal now, in the case
		* where no translation option covers a single-word span,
		* we leave the +inf in the matrix
		*	like in chart parsing we want each cell to contain the highest score
		*	of the full-span trOpt or the sum of scores of joining two smaller spans
		*/
	void CalcOptimisticScores();
};


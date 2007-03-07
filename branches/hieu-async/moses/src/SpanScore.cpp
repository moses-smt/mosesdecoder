// $Id: SpanScore.cpp 1034 2006-12-01 22:10:23Z hieuhoang1972 $

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

#include "SpanScore.h"
#include "WordsBitmap.h"

using namespace std;

SpanScore::~SpanScore()
{
	SpanScoreType::iterator iter;
	for (iter = m_matrices.begin() ; iter != m_matrices.end() ; ++iter)
	{
		SquareMatrix<float> *sqMatrix = *iter;
		delete sqMatrix;
	}
}

void SpanScore::Initialize(const vector<const DecodeStep*> &decodeStepList)
{
	m_matrices.resize(decodeStepList.size());
	for (size_t decodeStepId = 0 ; decodeStepId < m_matrices.size() ; ++decodeStepId)
	{
		SquareMatrix<float> *sqMatrix = new SquareMatrix<float>(m_size);
		sqMatrix->ResetScore(-numeric_limits<float>::infinity());
		m_matrices[decodeStepId] = sqMatrix;
	}
}

float SpanScore::GetFutureScore(const WordsBitmap &sourceCompleted) const
{
	float totalFutureScore		= 0.0f;
	SpanScoreType::const_iterator iterMap;

	for (size_t decodeStepId = 0 ; decodeStepId < m_matrices.size() ; ++decodeStepId)
	{
		size_t	start	= NOT_FOUND;
		for(size_t currPos = 0 ; currPos < sourceCompleted.GetSize() ; currPos++) 
		{
			if(sourceCompleted.GetValue(decodeStepId, currPos) == 0 && start == NOT_FOUND)
			{
				start = currPos;
			}
			if(sourceCompleted.GetValue(decodeStepId, currPos) == 1 && start != NOT_FOUND) 
			{
				totalFutureScore += GetScore(decodeStepId, start, currPos - 1);
				start = NOT_FOUND;
			}
		}
		if (start != NOT_FOUND)
		{
			totalFutureScore += GetScore(decodeStepId, start, sourceCompleted.GetSize() - 1);
		}
	}

	return totalFutureScore;
}

void SpanScore::CalcOptimisticScores()
{
	for (size_t decodeStepId = 0 ; decodeStepId < m_matrices.size() ; ++decodeStepId)
	{	
		SquareMatrix<float> &matrix = *m_matrices[decodeStepId];
		for(size_t startPos = 0; startPos < m_size ; ++startPos) 
		{
			for(size_t endPos = startPos+1; endPos < m_size ; ++endPos) 
			{
				for(size_t joinAt = startPos; joinAt < endPos ; joinAt++)  
				{
					float joinedScore = matrix.GetScore(startPos, joinAt)
														+ matrix.GetScore(joinAt+1, endPos);
					/* // uncomment to see the cell filling scheme
					TRACE_ERR( "[" <<startPos<<","<<endPos<<"] <-? ["<<startPos<<","<<joinAt<<"]+["<<joinAt+1<<","<<endPos
						<< "] (colStart: "<<colStart<<", diagShift: "<<diagShift<<")"<<endl);
					*/
					if (joinedScore > matrix.GetScore(startPos, endPos))
						matrix.SetScore(startPos, endPos, joinedScore);
				}
			}
		}
	}
}


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

#include "FutureScore.h"
#include "WordsBitmap.h"

using namespace std;

FutureScore::~FutureScore()
{
	FutureScoreType::iterator iter;
	for (iter = m_futureScore.begin() ; iter != m_futureScore.end() ; ++iter)
	{
		SquareMatrix *sqMatrix = iter->second;
		delete sqMatrix;
	}
}

void FutureScore::AddDecodeStep(const DecodeStep *decodeStep)
{
	assert(m_futureScore.find(decodeStep) == m_futureScore.end());
	SquareMatrix *sqMatrix = new SquareMatrix(m_size);
	sqMatrix->ResetScore(-numeric_limits<float>::infinity());
	m_futureScore[decodeStep] = sqMatrix;
}

float FutureScore::GetFutureScore(const WordsBitmap &sourceCompleted) const
{
	size_t	start				= NOT_FOUND;
	float totalFutureScore		= 0.0f;
	FutureScoreType::const_iterator iterMap;
	
	for (iterMap = m_futureScore.begin() ; iterMap != m_futureScore.end() ; ++iterMap)
	{
		const DecodeStep *decodeStep = iterMap->first;
		for(size_t currPos = 0 ; currPos < sourceCompleted.GetSize() ; currPos++) 
		{
			if(sourceCompleted.GetValue(decodeStep, currPos) == 0 && start == NOT_FOUND)
			{
				start = currPos;
			}
			if(sourceCompleted.GetValue(decodeStep, currPos) == 1 && start != NOT_FOUND) 
			{
				totalFutureScore += GetScore(decodeStep, start, currPos - 1);
				start = NOT_FOUND;
			}
		}
		if (start != NOT_FOUND)
		{
			totalFutureScore += GetScore(decodeStep, start, sourceCompleted.GetSize() - 1);
		}
	}

	return totalFutureScore;
}

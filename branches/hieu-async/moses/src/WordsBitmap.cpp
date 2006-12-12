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

#include <algorithm>
#include "WordsBitmap.h"
#include "StaticData.h"
#include "DecodeStep.h"
#include "FactorMask.h"

TO_STRING_BODY(WordsBitmap);

WordsBitmap::WordsBitmap(size_t size)
	:m_size	(size)
{
	const vector<DecodeStep*> &decodeStepList = StaticData::Instance()->GetDecodeStepList();
	m_bitmap.resize(decodeStepList.size());

	std::vector<DecodeStep*>::const_iterator iter;
	for (iter = decodeStepList.begin() ; iter != decodeStepList.end() ; ++iter)
	{
		size_t decodeStepId = (*iter)->GetId();
		bool *bitmap = (bool*) malloc(sizeof(bool) * size);
		m_bitmap[decodeStepId] = bitmap;
	}
	Initialize();
}

WordsBitmap::WordsBitmap(const WordsBitmap &copy)
	:m_size		(copy.m_size)
	,m_bitmap	(copy.m_bitmap)
{
	for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
	{
		bool *bitmap = (bool*) malloc(sizeof(bool) * m_size);
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			bitmap[pos] = copy.GetValue(decodeStepId, pos);
		}
		m_bitmap[decodeStepId] = bitmap;
	}
}

WordsBitmap::~WordsBitmap()
{
	for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
	{
		bool *bitmap = m_bitmap[decodeStepId];
		free(bitmap);
	}
}

void WordsBitmap::Initialize()
{
	for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
	{
		bool *bitmap = m_bitmap[decodeStepId];
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			bitmap[pos] = false;
		}
	}
}

int WordsBitmap::GetFutureDistortionScore(int lastPos) const 
{
	int ret = 0;
	for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
	{
		ret += GetFutureDistortionScore(decodeStepId, lastPos);
	}
	return ret;
}

int WordsBitmap::GetFutureDistortionScore(size_t decodeStepId, int lastPos) const 
{
	bool *bitmap = m_bitmap[decodeStepId];
	int sum=0;
	bool aim1	= 0
			,ai		= 0
			,aip1	= bitmap[0];
  
	for(size_t i=0;i<m_size;++i) {
		aim1 = ai;
		ai   = aip1;
		aip1 = (i+1==m_size || bitmap[i+1]);

#ifndef NDEBUG
		if( i>0 ) assert( aim1==(i==0||bitmap[i-1]==1));
		//assert( ai==a[i] );
		if( i+1<m_size ) assert( aip1==bitmap[i+1]);
#endif
		if((i==0||aim1)&&ai==0) {
			sum+=abs(lastPos-static_cast<int>(i)+1);
			//      sum+=getJumpCosts(lastPos,i,maxJumpWidth);
		}
		//    if(sum>1e5) return sum;
		if(i>0 && ai==0 && (i+1==m_size||aip1) ) 
			lastPos = (int) (i+1);
	}

	//  sum+=getJumpCosts(lastPos,as,maxJumpWidth);
	sum+=abs(lastPos-static_cast<int>(m_size)+1); //getCosts(lastPos,as);
	assert(sum>=0);

	return sum;
}

std::vector<size_t> WordsBitmap::GetCompressedRepresentation() const
{
	std::vector<size_t> ret;
	for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
	{
		std::vector<size_t> compressedRep = GetCompressedRepresentation(decodeStepId);
		std::copy(compressedRep.begin(), compressedRep.end() , std::inserter(ret, ret.end()) );
	}
	return ret;
}

std::vector<size_t> WordsBitmap::GetCompressedRepresentation(size_t decodeStepId) const
{
	bool *bitmap = m_bitmap[decodeStepId];

	std::vector<size_t> res(1 + (m_size >> (sizeof(int) + 3)), 0);
  size_t c=0; size_t x=0; size_t ci=0;
  for(size_t i=0;i<m_size;++i) {
    x |= (size_t)bitmap[i];
		x <<= 1;
		c++;
		if (c == sizeof(int)*8) {
			res[ci++] = x; x = 0;
		}
  }
  return res;
}

bool WordsBitmap::IsComplete(FactorType factorType) const
{
	for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
	{
		const DecodeStep &decodeStep = StaticData::Instance()->GetDecodeStep(decodeStepId);
		const FactorMask &outputFactorMask = decodeStep.GetCombinedOutputFactorMask();
		if (outputFactorMask[factorType] && IsComplete(decodeStep))
		{
			return true;
		}
	}
	return false;
}

bool WordsBitmap::IsComplete(const FactorMask &factorMask) const
{
	for (size_t currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; ++currFactor)
	{
		if (!IsComplete(currFactor))
			return false;
	}
	return true;
}

bool WordsBitmap::IsComplete(const DecodeStep &decodeStep) const
{
	return GetSize() == GetNumWordsCovered(decodeStep.GetId());
}

bool WordsBitmap::IsHierarchy(size_t decodeStepId, size_t startPos, size_t endPos) const
{
	if (decodeStepId == 0)
		return true;

	bool *bitmap = m_bitmap[decodeStepId - 1];
	for (size_t pos = startPos ; pos <= endPos ; ++pos)
	{
		if (!bitmap[pos])
			return false;
	}
	return true;
}

size_t WordsBitmap::GetStackIndex() const
{
	size_t ret = 0;
	for (size_t decodeStepId = 0 ; decodeStepId < m_bitmap.size() ; ++decodeStepId)
	{
		size_t wordsTranslated = GetNumWordsCovered(decodeStepId);
		ret += (size_t) pow((float)(GetSize()+1), (int) decodeStepId) * wordsTranslated;
	}
	return ret;
}

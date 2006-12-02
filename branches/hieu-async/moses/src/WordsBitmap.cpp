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

#include "WordsBitmap.h"

TO_STRING_BODY(WordsBitmap);

WordsBitmap::WordsBitmap(const std::list < DecodeStep* > &decodeStepList, size_t size)
	:m_size	(size)
{
	BitmapType::iterator iter;
	for (iter = m_bitmap.begin() ; iter != m_bitmap.end() ; ++iter)
	{
		bool *bitmap = iter->second;
		bitmap = (bool*) malloc(sizeof(bool) * size);
	}
	Initialize();
}

WordsBitmap::WordsBitmap(const WordsBitmap &copy)
	:m_size		(copy.m_size)
{
	BitmapType::iterator iter;
	for (iter = m_bitmap.begin() ; iter != m_bitmap.end() ; ++iter)
	{
		const DecodeStep *decodeStep =iter->first;
		bool *bitmap = iter->second;
		bitmap = (bool*) malloc(sizeof(bool) * m_size);
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			bitmap[pos] = copy.GetValue(decodeStep, pos);
		}
	}
}

WordsBitmap::~WordsBitmap()
{
	BitmapType::iterator iter;
	for (iter = m_bitmap.begin() ; iter != m_bitmap.end() ; ++iter)
	{
		bool *bitmap = iter->second;
		free(bitmap);
	}
}

void WordsBitmap::Initialize()
{
	BitmapType::iterator iter;
	for (iter = m_bitmap.begin() ; iter != m_bitmap.end() ; ++iter)
	{
		bool *bitmap = iter->second;
		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			bitmap[pos] = false;
		}
	}
}

int WordsBitmap::GetFutureCosts(const DecodeStep *decoderStep, int lastPos) const 
{
	bool *bitmap = GetBitmap(decoderStep);
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

	//	TRACE_ERR(sum<<"\n");

	return sum;
}


std::vector<size_t> WordsBitmap::GetCompressedRepresentation(const DecodeStep *decoderStep) const
{
	bool *bitmap = GetBitmap(decoderStep);

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


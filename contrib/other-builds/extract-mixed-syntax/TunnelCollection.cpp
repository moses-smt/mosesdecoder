/*
 *  TunnelCollection.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "TunnelCollection.h"
#include "Range.h"

using namespace std;

size_t TunnelCollection::NumUnalignedWord(size_t direction, size_t startPos, size_t endPos) const
{
	assert(startPos <= endPos);
	
	if (direction == 0)
		assert(endPos < alignedCountS.size());		
	else 
		assert(endPos < alignedCountT.size());

	size_t ret = 0; 
	for (size_t ind = startPos; ind <= endPos; ++ind)
	{
		if (direction == 0 && alignedCountS[ind] == 0)
		{
			ret++;
		}
		else if (direction == 1 && alignedCountT[ind] == 0)
		{
			ret++;
		}
		
	}
	
	return ret;
}

void TunnelCollection::Add(int startS, int endS, int startT, int endT)
{
	// m_phraseExist[startS][endS - startS].push_back(Tunnel(startT, endT));
	m_coll[startS][endS - startS].push_back(Tunnel(Range(startS, endS), Range(startT, endT)));
}


std::ostream& operator<<(std::ostream &out, const TunnelCollection &TunnelCollection)
{
	size_t size = TunnelCollection.GetSize();
	
	for (size_t startPos = 0; startPos < size; ++startPos)
	{
		for (size_t endPos = startPos; endPos < size; ++endPos)
		{
			const TunnelList &tunnelList = TunnelCollection.GetTunnels(startPos, endPos);
			TunnelList::const_iterator iter;
			for (iter = tunnelList.begin(); iter != tunnelList.end(); ++iter)
			{
				const Tunnel &tunnel = *iter;
				out << tunnel << " ";
				
			}
 		}
	}
	
	return out;
}



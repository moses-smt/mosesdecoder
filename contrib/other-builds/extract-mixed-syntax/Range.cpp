/*
 *  Range.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 22/02/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "Range.h"

using namespace std;

void Range::Merge(const Range &a, const Range &b)
{
	if (a.m_startPos == NOT_FOUND)
	{ // get the other regardless
		m_startPos = b.m_startPos;
	}
	else if (b.m_startPos == NOT_FOUND)
	{ 	
		m_startPos = a.m_startPos;
	}
	else
	{
		m_startPos = min(a.m_startPos, b.m_startPos);
	}

	if (a.m_endPos == NOT_FOUND)
	{ // get the other regardless
		m_endPos = b.m_endPos;
	}
	else if (b.m_endPos == NOT_FOUND)
	{ // do nothing		
		m_endPos = a.m_endPos;
	}
	else
	{
		m_endPos = max(a.m_endPos, b.m_endPos);
	}
	
	
}

int Range::Compare(const Range &other) const
{
	if (m_startPos < other.m_startPos)
		return -1;
	else if (m_startPos > other.m_startPos)
		return +1;
	else if (m_endPos < other.m_endPos)
		return -1;
	else if (m_endPos > other.m_endPos)
		return +1;
	
	return 0;
	
}

bool Range::Overlap(const Range &other) const
{
	if ( other.m_endPos < m_startPos || other.m_startPos > m_endPos) 
		return false;
	
	return true;	
}

std::ostream& operator<<(std::ostream &out, const Range &range)
{
	out << "[" << range.m_startPos << "-" << range.m_endPos << "]";
	return out;
}



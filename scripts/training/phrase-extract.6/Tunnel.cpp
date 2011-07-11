/*
 *  Tunnel.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Tunnel.h"

int Tunnel::Compare(const Tunnel &other) const
{
	int ret = 0;
	if (m_start != other.m_start)
		ret = (m_start < other.m_start) ? -1 : +1;
	
	if (ret == 0)
	{
		if (m_end != other.m_end)
			ret = (m_end < other.m_end) ? -1 : +1;
	}
	
	return ret;
}

int Tunnel::Compare(const Tunnel &other, size_t direction) const
{
	std::pair<size_t, size_t> thisHole(GetStart(direction), GetEnd(direction));
	std::pair<size_t, size_t> otherHole(other.GetStart(direction), other.GetEnd(direction));

	if (thisHole != otherHole)
		return (thisHole < otherHole) ? -1 : +1; 
	else
		return 0;
}


std::ostream& operator<<(std::ostream &out, const Tunnel &hole)
{
	out << "[" << hole.m_start[0] << "-" << hole.m_end[0] << "]"
			<< "=>" << "[" << hole.m_start[1] << "-" << hole.m_end[1] << "]";
	return out;
}


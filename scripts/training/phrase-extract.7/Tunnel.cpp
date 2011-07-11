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
	int ret = m_sourceRange.Compare(other.m_sourceRange);
	
	if (ret != 0)
		return ret;

	ret = m_targetRange.Compare(other.m_targetRange);
		
	return ret;
}

int Tunnel::Compare(const Tunnel &other, size_t direction) const
{
	const Range &thisRange = (direction == 0) ? m_sourceRange : m_targetRange;
	const Range &otherRange = (direction == 0) ? other.m_sourceRange : other.m_targetRange;
	
	int ret = thisRange.Compare(otherRange);
	return ret;
}

std::ostream& operator<<(std::ostream &out, const Tunnel &tunnel)
{
	out << tunnel.m_sourceRange << "==>" << tunnel.m_targetRange;
	return out;
}

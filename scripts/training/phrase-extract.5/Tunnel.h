#pragma once

/*
 *  Tunnel.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include <cassert>
#include <string>
#include <iostream>

class Tunnel
{
	friend std::ostream& operator<<(std::ostream&, const Tunnel&);

protected:
	std::vector<size_t> m_start, m_end;

public:
	Tunnel()
	:m_start(2)
	,m_end(2)
	{}
	
	Tunnel(const Tunnel &copy)
	:m_start(copy.m_start)
	,m_end(copy.m_end)
	{}
	
	Tunnel(size_t startS, size_t endS, size_t startT, size_t endT)
	:m_start(2)
	,m_end(2)
	{
		m_start[0] = startS;
		m_end[0] = endS;
		
		m_start[1] = startT;
		m_end[1] = endT;
	}
	
	size_t GetStart(size_t direction) const
	{ return m_start[direction]; }
	size_t GetEnd(size_t direction) const
	{ return m_end[direction]; }
	size_t GetSpan(size_t direction) const
	{ return m_end[direction] - m_start[direction] + 1; }
		
	int Compare(const Tunnel &other) const;
	int Compare(const Tunnel &other, size_t direction) const;
};

typedef std::vector<Tunnel> TunnelList;


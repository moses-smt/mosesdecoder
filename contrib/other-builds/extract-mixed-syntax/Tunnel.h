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
#include "Range.h"

	// for unaligned source terminal

class Tunnel
{
	friend std::ostream& operator<<(std::ostream&, const Tunnel&);

protected:
	
	Range m_sourceRange, m_targetRange;

public:
	Tunnel()
	{}
	
	Tunnel(const Tunnel &copy)
	:m_sourceRange(copy.m_sourceRange)
	,m_targetRange(copy.m_targetRange)
	{}
	
	Tunnel(const Range &sourceRange, const Range &targetRange)
	:m_sourceRange(sourceRange)
	,m_targetRange(targetRange)
	{}
	
	const Range &GetRange(size_t direction) const
	{ return (direction == 0) ? m_sourceRange : m_targetRange; }
		
	int Compare(const Tunnel &other) const;
	int Compare(const Tunnel &other, size_t direction) const;
};

typedef std::vector<Tunnel> TunnelList;


/*
 *  Range.h
 *  extract
 *
 *  Created by Hieu Hoang on 22/02/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
#pragma once
#include <string>
#include <iostream>
#include <limits>

#define NOT_FOUND 			std::numeric_limits<size_t>::max()

class Range
{
	friend std::ostream& operator<<(std::ostream&, const Range&);

	size_t m_startPos, m_endPos;
public:

	Range()
	:m_startPos(NOT_FOUND)
	,m_endPos(NOT_FOUND)
	{}
	
	Range(const Range &copy)
	:m_startPos(copy.m_startPos)
	,m_endPos(copy.m_endPos)
	{}

	Range(size_t startPos, size_t endPos)
	:m_startPos(startPos)
	,m_endPos(endPos)
	{}
	
	size_t GetStartPos() const
	{ return m_startPos; }
	size_t GetEndPos() const
	{ return m_endPos; }
	size_t GetWidth() const
	{ return m_endPos - m_startPos + 1; }

	void SetStartPos(size_t startPos)
	{ m_startPos = startPos; }
	void SetEndPos(size_t endPos)
	{ m_endPos = endPos; }
	
	void Merge(const Range &a, const Range &b);
	
	int Compare(const Range &other) const;

	bool Overlap(const Range &other) const;
	
	
};

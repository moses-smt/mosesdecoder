#pragma once

/*
 *  Hole.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <list>

class Hole
	{
	protected:
		int m_start, m_end, m_pos;
		
	public:
		Hole(const Hole &copy)
		:m_start(copy.m_start)
		,m_end(copy.m_end)
		{}
		Hole(int startPos, int endPos)
		:m_start(startPos)
		,m_end(endPos)
		{}
		
		int GetStart() const
		{ return m_start; }
		int GetEnd() const
		{ return m_end; }
		
		void SetPos(int pos)
		{ m_pos = pos; }
		int GetPos() const
		{ return m_pos; }
		
		bool Overlap(const Hole &otherHole) const
		{
			return ! ( otherHole.GetEnd()   < GetStart() || 
								otherHole.GetStart() > GetEnd() );
		}
		
		bool Neighbor(const Hole &otherHole) const
		{
			return ( otherHole.GetEnd()+1 == GetStart() || 
							otherHole.GetStart() == GetEnd()+1 ); 
		}
		
		
	};

typedef std::list<Hole> HoleList;

class HoleOrderer
	{
	public:
		bool operator()(const Hole* holeA, const Hole* holeB) const
		{
			assert(holeA->GetStart() != holeB->GetStart());
			return holeA->GetStart() < holeB->GetStart();
		}
	};

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
#include <vector>
#include <cassert>
#include <string>

class Hole
	{
	protected:
		std::vector<int> m_start, m_end, m_pos;
		std::vector<std::string> m_label;

	public:
		Hole()
		:m_start(2)
		,m_end(2)
		,m_pos(2)
		,m_label(2)
		{}
		
		Hole(const Hole &copy)
		:m_start(copy.m_start)
		,m_end(copy.m_end)
		,m_pos(copy.m_pos)
		,m_label(copy.m_label)
		{}
		
		Hole(int startS, int endS, int startT, int endT)
		:m_start(2)
		,m_end(2)
		,m_pos(2)
		,m_label(2)
		{
			m_start[0] = startS;
			m_end[0] = endS;
			
			m_start[1] = startT;
			m_end[1] = endT;
		}
		
		int GetStart(size_t direction) const
		{ return m_start[direction]; }
		int GetEnd(size_t direction) const
		{ return m_end[direction]; }
		
		void SetPos(int pos, size_t direction)
		{ m_pos[direction] = pos; }
		int GetPos(size_t direction) const
		{ return m_pos[direction]; }
		
		void SetLabel(const std::string &label, size_t direction)
		{ m_label[direction] = label; }
		const std::string &GetLabel(size_t direction) const
		{ return m_label[direction]; }
		
		bool Overlap(const Hole &otherHole, size_t direction) const
		{
			return ! ( otherHole.GetEnd(direction)   < GetStart(direction) || 
								otherHole.GetStart(direction) > GetEnd(direction) );
		}
		
		bool Neighbor(const Hole &otherHole, size_t direction) const
		{
			return ( otherHole.GetEnd(direction)+1 == GetStart(direction) || 
							otherHole.GetStart(direction) == GetEnd(direction)+1 ); 
		}
		
	};

typedef std::list<Hole> HoleList;

class HoleSourceOrderer
	{
	public:
		bool operator()(const Hole* holeA, const Hole* holeB) const
		{
			assert(holeA->GetStart(0) != holeB->GetStart(0));
			return holeA->GetStart(0) < holeB->GetStart(0);
		}
	};


#pragma once
/*
 *  HoleCollection.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include "Hole.h"

class HoleCollection
	{
	protected:
		HoleList m_holes;
		std::vector<Hole*> m_sortedSourceHoles;
		
	public:
		HoleCollection()
		{}
		HoleCollection(const HoleCollection &copy)
		:m_holes(copy.m_holes)
		{} // don't copy sorted target holes. messes up sorting fn
		
		const HoleList &GetHoles() const
		{ return m_holes; }		
		HoleList &GetHoles()
		{ return m_holes; }
		std::vector<Hole*> &GetSortedSourceHoles()
		{ return m_sortedSourceHoles; }
		
		void Add(int startT, int endT, int startS, int endS)
		{
			m_holes.push_back(Hole(startS, endS, startT, endT));
		}
		
		bool OverlapSource(const Hole &sourceHole) const
		{
			HoleList::const_iterator iter;
			for (iter = m_holes.begin(); iter != m_holes.end(); ++iter)
			{
				const Hole &currHole = *iter;
				if (currHole.Overlap(sourceHole, 0))
					return true;
			}
			return false;
		}
		
		bool ConsecSource(const Hole &sourceHole) const
		{
			HoleList::const_iterator iter;
			for (iter = m_holes.begin(); iter != m_holes.end(); ++iter)
			{
				const Hole &currHole = *iter;
				if (currHole.Neighbor(sourceHole, 0))
					return true;
			}
			return false;
		}
		
		void SortSourceHoles();
		
	};

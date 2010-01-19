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
		HoleList m_sourceHoles, m_targetHoles;
		std::vector<Hole*> m_sortedSourceHoles;
		
	public:
		HoleCollection()
		{}
		HoleCollection(const HoleCollection &copy)
		:m_sourceHoles(copy.m_sourceHoles)
		,m_targetHoles(copy.m_targetHoles)
		{} // don't copy sorted target holes. messes up sorting fn
		
		const HoleList &GetSourceHoles() const
		{ return m_sourceHoles; }
		const HoleList &GetTargetHoles() const
		{ return m_targetHoles; }
		
		HoleList &GetSourceHoles()
		{ return m_sourceHoles; }
		HoleList &GetTargetHoles()
		{ return m_targetHoles; }
		std::vector<Hole*> &GetSortedSourceHoles()
		{ return m_sortedSourceHoles; }
		
		void Add(int startT, int endT, int startS, int endS)
		{
			m_sourceHoles.push_back(Hole(startS, endS));
			m_targetHoles.push_back(Hole(startT, endT));
		}
		
		bool OverlapSource(const Hole &sourceHole) const
		{
			HoleList::const_iterator iter;
			for (iter = m_sourceHoles.begin(); iter != m_sourceHoles.end(); ++iter)
			{
				const Hole &currHole = *iter;
				if (currHole.Overlap(sourceHole))
					return true;
			}
			return false;
		}
		
		bool ConsecSource(const Hole &sourceHole) const
		{
			HoleList::const_iterator iter;
			for (iter = m_sourceHoles.begin(); iter != m_sourceHoles.end(); ++iter)
			{
				const Hole &currHole = *iter;
				if (currHole.Neighbor(sourceHole))
					return true;
			}
			return false;
		}
		
		void SortSourceHoles();
		
	};

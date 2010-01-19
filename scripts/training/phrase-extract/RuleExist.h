#pragma once
/*
 *  RuleExist.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include "Hole.h"

// reposity of extracted phrase pairs
// which are potential holes in larger phrase pairs
class RuleExist
	{
	protected:
		std::vector< std::vector<HoleList> > m_phraseExist;
		// indexed by source pos. and source length 
		// maps to list of holes where <int, int> are target pos
		
	public:
		RuleExist(size_t size)
		:m_phraseExist(size)
		{
			// size is the length of the source sentence
			for (size_t pos = 0; pos < size; ++pos)
			{
				// create empty hole lists
				std::vector<HoleList> &endVec = m_phraseExist[pos];
				endVec.resize(size - pos);
			}
		}
		
		void Add(int startT, int endT, int startS, int endS)
		{
			// m_phraseExist[startS][endS - startS].push_back(Hole(startT, endT));
			m_phraseExist[startT][endT - startT].push_back(Hole(startS, endS));
		}
		//const HoleList &GetTargetHoles(int startS, int endS) const
		//{
		//	const HoleList &targetHoles = m_phraseExist[startS][endS - startS];
		//	return targetHoles;
		//}
		const HoleList &GetSourceHoles(int startT, int endT) const
		{
			const HoleList &sourceHoles = m_phraseExist[startT][endT - startT];
			return sourceHoles;
		}
		
	};


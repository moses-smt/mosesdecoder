#pragma once
/*
 *  TunnelCollection.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include "Tunnel.h"

// reposity of extracted phrase pairs
// which are potential tunnels in larger phrase pairs
class TunnelCollection
	{
		friend std::ostream& operator<<(std::ostream&, const TunnelCollection&);

	protected:
		std::vector< std::vector<TunnelList> > m_coll;
		// indexed by source pos. and source length 
		// maps to list of tunnels where <int, int> are target pos

	public:
		std::vector<int> alignedCountS, alignedCountT;

		TunnelCollection(const TunnelCollection &);

		TunnelCollection(size_t size)
		:m_coll(size)
		{
			// size is the length of the source sentence
			for (size_t pos = 0; pos < size; ++pos)
			{
				// create empty tunnel lists
				std::vector<TunnelList> &endVec = m_coll[pos];
				endVec.resize(size - pos);
			}
		}
		
		void Add(int startS, int endS, int startT, int endT);

		//const TunnelList &GetTargetHoles(int startS, int endS) const
		//{
		//	const TunnelList &targetHoles = m_phraseExist[startS][endS - startS];
		//	return targetHoles;
		//}
		const TunnelList &GetTunnels(int startS, int endS) const
		{
			const TunnelList &sourceHoles = m_coll[startS][endS - startS];
			return sourceHoles;
		}
		
		const size_t GetSize() const
		{ return m_coll.size(); }
		
		size_t NumUnalignedWord(size_t direction, size_t startPos, size_t endPos) const;


	};


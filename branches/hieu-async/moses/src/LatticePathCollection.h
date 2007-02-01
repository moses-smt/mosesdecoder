// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <set>
#include <iostream>
#include "LatticePath.h"

struct CompareLatticePathCollection
{
	bool operator()(const LatticePath* pathA, const LatticePath* pathB) const
	{
		return (pathA->GetTotalScore() > pathB->GetTotalScore());
	}
};

/** priority queue used in Manager to store list of contenders for N-Best list.
	* Stored in order of total score so that the best path can just be popped from the top
	*/
class LatticePathCollection
{
	friend std::ostream& operator<<(std::ostream&, const LatticePathCollection&);
protected:
	typedef std::multiset<LatticePath*, CompareLatticePathCollection> CollectionType;
	CollectionType m_collection;
	std::set< std::vector<const Hypothesis *> > m_uniquePath; 
		// not sure if really needed. does the partitioning algorithm create duplicate paths ?

public:	
	//iterator begin() { return m_collection.begin(); }
	LatticePath *pop()
	{
		LatticePath *top = *m_collection.begin();

		// Detach
		// delete from m_uniquePath as well
		const std::vector<const Hypothesis *> &edges = top->GetEdges();
		m_uniquePath.erase(edges);

		m_collection.erase(m_collection.begin());

		return top;
	}

	~LatticePathCollection()
	{
		// clean up
		RemoveAllInColl(m_collection);
	}
	
	//! add a new entry into collection
	void Add(LatticePath *latticePath)
	{
		const std::vector<const Hypothesis *> &edges = latticePath->GetEdges();
		if ( m_uniquePath.insert(edges).second )
		{ // path not yet in collection
			m_collection.insert(latticePath);
		}
		else
		{ // path already in there
			assert(false);
			delete latticePath;
		}
	}
	
	size_t GetSize() const
	{
		return m_collection.size();
	}

	void Prune(size_t newSize);
};

inline std::ostream& operator<<(std::ostream& out, const LatticePathCollection& pathColl)
{
	LatticePathCollection::CollectionType::const_iterator iter;
	
	for (iter = pathColl.m_collection.begin() ; iter != pathColl.m_collection.end() ; ++iter)
	{
		const LatticePath &path = **iter;
		out << path << std::endl;
	}
	return out;
}


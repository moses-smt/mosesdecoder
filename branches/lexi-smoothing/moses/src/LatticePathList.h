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

#include <list>
#include <set>
#include "LatticePath.h"


class LatticePathList
{
protected:
	 std::list<const LatticePath*> m_collection;
	 std::set< std::vector<const Hypothesis *> > m_uniquePath;
public:
	// iters
	typedef std::list<const LatticePath*>::iterator iterator;
	typedef std::list<const LatticePath*>::const_iterator const_iterator;
	
	iterator begin() { return m_collection.begin(); }
	iterator end() { return m_collection.end(); }
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }

	~LatticePathList()
	{
		// clean up
		RemoveAllInColl(m_collection);
	}

	//! add a new entry into collection
	bool Add(LatticePath *latticePath)
	{
		// make sure it not duplicated
		const std::vector<const Hypothesis *> &edges = latticePath->GetEdges();
		if ( m_uniquePath.insert(edges).second )
		{
			m_collection.push_back(latticePath);
			return true;
		}
		else
			return false;
	}

	size_t GetSize() const
	{
		return m_collection.size();
	}
};


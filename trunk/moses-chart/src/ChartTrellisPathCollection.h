// $Id: PhraseDictionaryNewFormat.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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
#include "ChartTrellisPath.h"

namespace MosesChart
{

class TrellisPath;

struct CompareTrellisPathCollection
{
	bool operator()(const TrellisPath* pathA, const TrellisPath* pathB) const
	{
		return (pathA->GetTotalScore() > pathB->GetTotalScore());
	}
};

class TrellisPathCollection
{
protected:
	typedef std::multiset<TrellisPath*, CompareTrellisPathCollection> CollectionType;
	CollectionType m_collection;

public:
	~TrellisPathCollection();

	size_t GetSize() const
	{ return m_collection.size(); }

	void Add(TrellisPath *path);
	void Prune(size_t newSize);

	TrellisPath *pop()
	{
		TrellisPath *top = *m_collection.begin();

		// Detach
		m_collection.erase(m_collection.begin());
		return top;
	}

};


}


// $Id$
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
#include <queue>
#include <vector>
#include <set>
#include "QueueEntry.h"

namespace MosesChart
{
	
class QueueEntryUniqueOrderer
{
public:
	bool operator()(const QueueEntry* entryA, const QueueEntry* entryB) const
	{
		return (*entryA) < (*entryB);
	}
};

class QueueEntryScoreOrderer
{
public:
	bool operator()(const QueueEntry* entryA, const QueueEntry* entryB) const
	{
		return (entryA->GetCombinedScore() < entryB->GetCombinedScore());
	}
};

	
class Cube
{
protected:	
	typedef std::set<QueueEntry*, QueueEntryUniqueOrderer> UniqueCubeEntry;
	UniqueCubeEntry m_uniqueEntry;
	
	typedef std::priority_queue<QueueEntry*, std::vector<QueueEntry*>, QueueEntryScoreOrderer> SortedByScore;
	SortedByScore m_sortedByScore;
	

public:
	~Cube();
	bool IsEmpty() const
	{ return m_sortedByScore.empty(); }

	QueueEntry *Pop();
	bool Add(QueueEntry *queueEntry);
};


};


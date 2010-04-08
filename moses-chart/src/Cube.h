/*
 *  Cube.h
 *  moses-chart
 *
 *  Created by Hieu Hoang on 18/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
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

	void clear();
};


};


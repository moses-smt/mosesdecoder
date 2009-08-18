/*
 *  Cube.h
 *  moses-chart
 *
 *  Created by Hieu Hoang on 18/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <set>
#include "QueueEntry.h"

namespace MosesChart
{
class Cube
{
protected:	
	typedef std::set<QueueEntry*, QueueEntryOrderer> CollType;
	CollType m_coll;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	
	bool IsEmpty() const
	{ return m_coll.empty(); }

	bool Add(QueueEntry *queueEntry);

	bool Erase(QueueEntry *queueEntry)
	{
		return m_coll.erase(queueEntry);
	}
};


};
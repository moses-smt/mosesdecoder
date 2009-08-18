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
	bool IsEmpty() const
	{ return m_coll.empty(); }

	QueueEntry *Pop()
	{
		QueueEntry *entry = *m_coll.begin();
		m_coll.erase(m_coll.begin());
		return entry;
	}
	
	bool Add(QueueEntry *queueEntry);
};


};


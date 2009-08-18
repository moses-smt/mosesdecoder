/*
 *  Cube.cpp
 *  moses-chart
 *
 *  Created by Hieu Hoang on 18/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Cube.h"

using namespace std;

namespace MosesChart
{
bool Cube::Add(QueueEntry *queueEntry)
{
	std::pair<set<QueueEntry*, QueueEntryOrderer>::iterator, bool> inserted = m_coll.insert(queueEntry);

	return inserted.second;
}


}


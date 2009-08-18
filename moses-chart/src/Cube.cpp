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
	m_sortedByScore.push(queueEntry);

	return true;
}

QueueEntry *Cube::Pop()
{
	QueueEntry *entry = m_sortedByScore.top();
	m_sortedByScore.pop();
	return entry;
}
	
}


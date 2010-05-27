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

#include "Cube.h"

using namespace std;

namespace MosesChart
{
Cube::~Cube()
{
	clear();
}

void
Cube::clear()
{
	while (!m_sortedByScore.empty())
	{
		m_sortedByScore.pop();
	}

	UniqueCubeEntry::iterator iter;
	for (iter = m_uniqueEntry.begin(); iter != m_uniqueEntry.end(); ++iter)
	{
		QueueEntry *entry = *iter;
		delete entry;
	}
	m_uniqueEntry.clear();
}

bool Cube::Add(QueueEntry *queueEntry)
{
	pair<UniqueCubeEntry::iterator, bool> inserted = m_uniqueEntry.insert(queueEntry);
	
	if (inserted.second)
	{ // inserted
		m_sortedByScore.push(queueEntry);
	}
	else
	{ // already there
		//cerr << "already there\n";
		delete queueEntry;
	}
	
	//assert(m_uniqueEntry.size() == m_sortedByScore.size());
	
	return inserted.second;
}

QueueEntry *Cube::Pop()
{
	QueueEntry *entry = m_sortedByScore.top();
	m_sortedByScore.pop();

	return entry;
}
	
}


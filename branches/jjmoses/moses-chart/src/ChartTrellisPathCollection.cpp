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

#include "ChartTrellisPathCollection.h"
#include "ChartTrellisPath.h"

namespace MosesChart
{

TrellisPathCollection::~TrellisPathCollection()
{
	// clean up
	Moses::RemoveAllInColl(m_collection);
}

void TrellisPathCollection::Add(TrellisPath *path)
{
	m_collection.insert(path);
}

void TrellisPathCollection::Prune(size_t newSize)
{
	size_t currSize = m_collection.size(); 

	if (currSize <= newSize)
		return; // don't need to prune

	CollectionType::reverse_iterator iterRev;
	for (iterRev = m_collection.rbegin() ; iterRev != m_collection.rend() ; ++iterRev)
	{
		TrellisPath *trellisPath = *iterRev;
		delete trellisPath;
		
		currSize--;
		if (currSize == newSize)
			break;
	}
	
	// delete path in m_collection
	CollectionType::iterator iter = m_collection.begin();
	for (size_t i = 0 ; i < newSize ; ++i)
		iter++;
	
	m_collection.erase(iter, m_collection.end());
}

}


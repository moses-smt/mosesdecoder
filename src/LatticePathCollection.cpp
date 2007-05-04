#include "LatticePathCollection.h"

void LatticePathCollection::Prune(size_t newSize)
{
	size_t currSize = m_collection.size(); 

	if (currSize <= newSize)
		return; // don't need to prune

	CollectionType::reverse_iterator iterRev;
	for (iterRev = m_collection.rbegin() ; iterRev != m_collection.rend() ; ++iterRev)
	{
		LatticePath *latticePath = *iterRev;
		delete latticePath;
		
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


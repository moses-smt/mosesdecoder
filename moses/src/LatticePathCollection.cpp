
#include "LatticePathCollection.h"

void LatticePathCollection::Prune(size_t newSize)
{
	assert( m_collection.size() == m_uniquePath.size() );
	if (m_collection.size() <= newSize)
		return; // don't need to prune

	CollectionType::reverse_iterator iterRev;
	for (iterRev = m_collection.rbegin() ; iterRev != m_collection.rend() ; ++iterRev)
	{
		LatticePath *latticePath = *iterRev;

		// delete path in m_uniquePath
		delete latticePath;
		if (m_collection.size() == newSize)
			break;
	}
	
	// delete path in m_collection
	CollectionType::iterator iter = m_collection.begin();
	for (size_t i = 0 ; i < newSize ; ++i)
		iter++;
	
	m_collection.erase(iter, m_collection.end());

	assert( m_collection.size() == m_uniquePath.size() );

}


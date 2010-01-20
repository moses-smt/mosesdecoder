/*
 *  HoleCollection.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <algorithm>
#include "HoleCollection.h"

using namespace std;

void HoleCollection::SortSourceHoles()
{
	assert(m_sortedSourceHoles.size() == 0);
	
	// add
	HoleList::iterator iter;
	for (iter = m_sourceHoles.begin(); iter != m_sourceHoles.end(); ++iter)
	{
		Hole &currHole = *iter;
		m_sortedSourceHoles.push_back(&currHole);
	}
	
	// sort
	std::sort(m_sortedSourceHoles.begin(), m_sortedSourceHoles.end(), HoleOrderer());
}

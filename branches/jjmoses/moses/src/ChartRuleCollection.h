// $Id: ChartRuleCollection.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#pragma once

#include <queue>
#include <vector>
#include <list>
#include <set>
#include "ChartRule.h"
#include "TargetPhrase.h"
#include "Util.h"
#include "TargetPhraseCollection.h"
#include "ObjectPool.h"

namespace Moses
{
//! a list of target phrases that is trsnalated from the same source phrase
class ChartRuleCollection
{
	friend std::ostream& operator<<(std::ostream&, const ChartRuleCollection&);

protected:
#ifdef USE_HYPO_POOL
		static ObjectPool<ChartRuleCollection> s_objectPool;
#endif
	std::vector<ChartRule*> m_collection;
	float m_scoreThreshold;
public:	
	// iters
	typedef std::vector<ChartRule*>::iterator iterator;
	typedef std::vector<ChartRule*>::const_iterator const_iterator;
	
	iterator begin() { return m_collection.begin(); }
	iterator end() { return m_collection.end(); }
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }

#ifdef USE_HYPO_POOL
	void *operator new(size_t num_bytes)
	{
		void *ptr = s_objectPool.getPtr();
		return ptr;
	}

	static void Delete(ChartRuleCollection *obj)
	{
		s_objectPool.freeObject(obj);
	}
#else
	static void Delete(ChartRuleCollection *obj)
	{
		delete obj;
	}
#endif

	ChartRuleCollection();
	~ChartRuleCollection();

	const ChartRule &Get(size_t ind) const
	{
		return *m_collection[ind];
	}

	//! divide collection into 2 buckets using std::nth_element, the top & bottom according to table limit
//	void Sort(size_t tableLimit);

	//! number of target phrases in this collection
	size_t GetSize() const
	{
		return m_collection.size();
	}
	//! wether collection has any phrases
	bool IsEmpty() const
	{ 
		return m_collection.empty();
	}	

	void Add(const Moses::TargetPhraseCollection &targetPhraseCollection
					, const WordConsumed &wordConsumed
					, bool ruleLimit
					, size_t tableLimit);
	
	void CreateChartRules(size_t ruleLimit);
};

}

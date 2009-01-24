// $Id: ChartRuleCollection.h 552 2009-01-09 14:05:34Z hieu $

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

#include <vector>
#include <list>
#include <set>
#include "ChartRule.h"
#include "TargetPhrase.h"
#include "Util.h"
#include "TargetPhraseCollection.h"

namespace Moses
{

//! a list of target phrases that is trsnalated from the same source phrase
class ChartRuleCollection
{
	friend std::ostream& operator<<(std::ostream&, const ChartRuleCollection&);

protected:
	std::vector<ChartRule*> m_collection;
	std::set<std::vector<WordsConsumed> > m_wordsConsumed;

public:	
	// iters
	typedef std::vector<ChartRule*>::iterator iterator;
	typedef std::vector<ChartRule*>::const_iterator const_iterator;
	
	iterator begin() { return m_collection.begin(); }
	iterator end() { return m_collection.end(); }
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }
	
	~ChartRuleCollection();

	const ChartRule &Get(size_t ind) const
	{
		return *m_collection[ind];
	}

	//! divide collection into 2 buckets using std::nth_element, the top & bottom according to table limit
	void Sort(size_t tableLimit);

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
					, const std::vector<WordsConsumed> &wordsConsumed
					, bool adhereTableLimit
					, size_t tableLimit);
	
	void Prune(size_t tableLimit);
};

}

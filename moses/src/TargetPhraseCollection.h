// $Id: TargetPhraseCollection.h 3078 2010-04-08 17:16:10Z hieuhoang1972 $

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

#ifndef moses_TargetPhraseCollection_h
#define moses_TargetPhraseCollection_h

#include <vector>
#include "TargetPhrase.h"
#include "Util.h"

namespace Moses
{

//! a list of target phrases that is translated from the same source phrase
class TargetPhraseCollection
{
protected:
	std::vector<TargetPhrase*> m_collection;
	
public:	
	// iters
	typedef std::vector<TargetPhrase*>::iterator iterator;
	typedef std::vector<TargetPhrase*>::const_iterator const_iterator;
	
	iterator begin() { return m_collection.begin(); }
	iterator end() { return m_collection.end(); }
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }
	
	~TargetPhraseCollection()
	{
			RemoveAllInColl(m_collection);
	}

	//! divide collection into 2 buckets using std::nth_element, the top & bottom according to table limit
	void NthElement(size_t tableLimit);

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
	//! add a new entry into collection
	void Add(TargetPhrase *targetPhrase)
	{
		m_collection.push_back(targetPhrase);
	}
	
	void Prune(bool adhereTableLimit, size_t tableLimit);

};

}

#endif

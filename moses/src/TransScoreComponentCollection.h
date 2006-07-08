// $Id$

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

#include <iostream>
#include <map>
#include <assert.h>
#include "TransScoreComponent.h"
#include "PhraseDictionary.h"

class TransScoreComponentCollection : public std::map<size_t, TransScoreComponent*>
{
public:
	TransScoreComponent &GetTransScoreComponent(size_t index)
	{
		TransScoreComponentCollection::iterator iter = find(index);
		assert(iter != end());
		return *iter->second;
	}
	const TransScoreComponent &GetTransScoreComponent(size_t index) const
	{
		return const_cast<TransScoreComponentCollection*>(this)->GetTransScoreComponent(index);
	}
	TransScoreComponent &Add(const TransScoreComponent &transScoreComponent)
	{
		size_t idPhraseDictionary = transScoreComponent.GetPhraseDictionaryId();
		TransScoreComponentCollection::iterator iter = find(idPhraseDictionary);
		if (iter != end())
		{ // already have scores for this phrase table. delete it 1st
			delete iter->second;
			erase(iter);
		}
		// add new into same place
		TransScoreComponent *newTransScoreComponent = new TransScoreComponent(transScoreComponent);
		operator[](idPhraseDictionary) = newTransScoreComponent;
		
		return *newTransScoreComponent;
	}
	TransScoreComponent &Add(const PhraseDictionary &phraseDictionary)
	{
		return Add(TransScoreComponent(&phraseDictionary));
	}
};

inline std::ostream& operator<<(std::ostream &out, const TransScoreComponentCollection &transScoreComponentColl)
{
	TransScoreComponentCollection::const_iterator iter;
	for (iter = transScoreComponentColl.begin() ; iter != transScoreComponentColl.end() ; ++iter)
	{
		size_t idTrans = iter->first;
		const TransScoreComponent &transScoreComponent = *iter->second;
		out << "[" << idTrans << "=" << transScoreComponent << "] ";
	}
	return out;
}


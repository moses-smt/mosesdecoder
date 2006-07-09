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
#include "ScoreComponent.h"
#include "PhraseDictionary.h"

class ScoreComponentCollection : public std::map<const PhraseDictionary *, ScoreComponent*>
{
public:
	ScoreComponentCollection()
	{
	}
	ScoreComponentCollection(const ScoreComponentCollection &copy)
	{
		ScoreComponentCollection::const_iterator iter;
		for (iter = copy.begin() ; iter != copy.end() ; ++iter)
		{
			const ScoreComponent *origScoreComponent = iter->second;
			Add(*origScoreComponent);
		}
	}

	~ScoreComponentCollection()
	{ // ??? memory leak but double free
		TRACE_ERR(this << std::endl);
		ScoreComponentCollection::iterator iter;
		for (iter = begin() ; iter != end() ; ++iter)
		{
			TRACE_ERR(iter->second << std::endl);
			TRACE_ERR(*iter->second << std::endl);			
			delete iter->second;
		}				
	}
	
	ScoreComponent &GetScoreComponent(const PhraseDictionary *phraseDictionary)
	{
		ScoreComponentCollection::iterator iter = find(phraseDictionary);
		assert(iter != end());
		return *iter->second;
	}
	
	const ScoreComponent &GetScoreComponent(const PhraseDictionary *phraseDictionary) const
	{
		return const_cast<ScoreComponentCollection*>(this)->GetScoreComponent(phraseDictionary);
	}
	
	ScoreComponent &Add(const ScoreComponent &transScoreComponent)
	{
		const PhraseDictionary *phraseDictionary = transScoreComponent.GetPhraseDictionary();
		ScoreComponentCollection::iterator iter = find(phraseDictionary);
		if (iter != end())
		{ // already have scores for this phrase table. delete it 1st
			delete iter->second;
			erase(iter);
		}
		// add new into same place
		ScoreComponent *newScoreComponent = new ScoreComponent(transScoreComponent);
		operator[](phraseDictionary) = newScoreComponent;
		
		return *newScoreComponent;
	}
	ScoreComponent &Add(const PhraseDictionary *phraseDictionary)
	{
		ScoreComponentCollection::iterator iter = find(phraseDictionary);
		if (iter != end())
		{ // already have scores for this phrase table. delete it 1st
			delete iter->second;
			erase(iter);
		}
		// add new into same place
		ScoreComponent *newScoreComponent = new ScoreComponent(phraseDictionary);
		operator[](phraseDictionary) = newScoreComponent;
		
		return *newScoreComponent;
	}
};

inline std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &transScoreComponentColl)
{
	ScoreComponentCollection::const_iterator iter;
	for (iter = transScoreComponentColl.begin() ; iter != transScoreComponentColl.end() ; ++iter)
	{
		const PhraseDictionary *phraseDictionary = iter->first;
		const ScoreComponent &transScoreComponent = *iter->second;
		out << "[" << phraseDictionary->GetId() << "=" << transScoreComponent << "] ";
	}
	return out;
}


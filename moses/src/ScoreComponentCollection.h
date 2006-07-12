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

class PhraseDictionary;

class ScoreComponentCollection : public std::map<const PhraseDictionary *, ScoreComponent>
{
public:
	ScoreComponent &GetScoreComponent(const PhraseDictionary *index)
	{
		ScoreComponentCollection::iterator iter = find(index);
		assert(iter != end());
		return iter->second;
	}
	const ScoreComponent &GetScoreComponent(const PhraseDictionary *phraseDictionary) const
	{
		return const_cast<ScoreComponentCollection*>(this)->GetScoreComponent(phraseDictionary);
	}
	ScoreComponent &Add(const ScoreComponent &transScoreComponent)
	{
		const PhraseDictionary *phraseDictionary = transScoreComponent.GetPhraseDictionary();
		return operator[](phraseDictionary) = transScoreComponent;
	}
	ScoreComponent &Add(const PhraseDictionary *phraseDictionary)
	{
		return Add(ScoreComponent(phraseDictionary));
	}
};

inline std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &transScoreComponentColl)
{
	ScoreComponentCollection::const_iterator iter;
	for (iter = transScoreComponentColl.begin() ; iter != transScoreComponentColl.end() ; ++iter)
	{
		const ScoreComponent &transScoreComponent = iter->second;
		out << "[" << transScoreComponent << "] ";
	}
	return out;
}


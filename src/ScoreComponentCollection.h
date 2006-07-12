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

class Dictionary;

class ScoreComponentCollection : public std::map<const Dictionary*, ScoreComponent>
{
public:
	ScoreComponent &GetScoreComponent(const Dictionary *dictionary)
	{
		ScoreComponentCollection::iterator iter = find(dictionary);
		assert(iter != end());
		return iter->second;
	}
	const ScoreComponent &GetScoreComponent(const Dictionary *dictionary) const
	{
		return const_cast<ScoreComponentCollection*>(this)->GetScoreComponent(dictionary);
	}
	void Remove(const Dictionary *dictionary)
	{
		erase(dictionary);
	}
	ScoreComponent &Add(const ScoreComponent &scoreComponent)
	{
		const Dictionary *dictionary = scoreComponent.GetDictionary();
		return operator[](dictionary) = scoreComponent;
	}
	ScoreComponent &Add(const Dictionary *dictionary)
	{
		return Add(ScoreComponent(dictionary));
	}
};

inline std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &scoreComponentColl)
{
	ScoreComponentCollection::const_iterator iter;
	for (iter = scoreComponentColl.begin() ; iter != scoreComponentColl.end() ; ++iter)
	{
		const ScoreComponent &scoreComponent = iter->second;
		out << "[" << scoreComponent << "] ";
	}
	return out;
}


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
#include <set>
#include <assert.h>
#include "ScoreComponent.h"

class Dictionary;

class ScoreComponentCollection : public std::set<ScoreComponent>
{
public:
	ScoreComponent &GetScoreComponent(const Dictionary *index)
	{
		ScoreComponentCollection::iterator iter = find(index);
		assert(iter != end());
		return *iter;
	}
	const ScoreComponent &GetScoreComponent(const Dictionary *dictionary) const
	{
		return const_cast<ScoreComponentCollection*>(this)->GetScoreComponent(dictionary);
	}
	void Remove(const ScoreComponent &transScoreComponent)
	{
		erase(transScoreComponent);
	}

	ScoreComponent &Add(const ScoreComponent &transScoreComponent)
	{
		iterator iter = find(transScoreComponent);
		assert(iter == end());
		std::pair<iterator, bool> added = insert(transScoreComponent);
		return *added.first;
	}
	ScoreComponent &Add(const Dictionary *dictionary)
	{
		return Add(ScoreComponent(dictionary));
	}
};

inline std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &transScoreComponentColl)
{
	ScoreComponentCollection::const_iterator iter;
	for (iter = transScoreComponentColl.begin() ; iter != transScoreComponentColl.end() ; ++iter)
	{
		const ScoreComponent &transScoreComponent = *iter;
		out << "[" << transScoreComponent << "] ";
	}
	return out;
}


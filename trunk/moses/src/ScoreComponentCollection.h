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
#include <vector>
#include <list>
#include <assert.h>
#include "ScoreComponent.h"

class Dictionary;

class ScoreComponentCollection
{
	friend std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &scoreComponentColl);
private:
	std::map<const Dictionary*, ScoreComponent> m_scores;
public:
	typedef std::map<const Dictionary*, ScoreComponent>::iterator iterator;
	typedef std::map<const Dictionary*, ScoreComponent>::const_iterator const_iterator;
	iterator begin() { return m_scores.begin(); }
	iterator end() { return m_scores.end(); }
	const_iterator begin() const { return m_scores.begin(); }
	const_iterator end() const { return m_scores.end(); }
	
	ScoreComponent &GetScoreComponent(const Dictionary *dictionary)
	{
		iterator iter = m_scores.find(dictionary);
		assert(iter != m_scores.end());
		return iter->second;
	}

	const ScoreComponent &GetScoreComponent(const Dictionary *dictionary) const
	{
		return const_cast<ScoreComponentCollection*>(this)->GetScoreComponent(dictionary);
	}

	void Remove(const Dictionary *dictionary)
	{
		m_scores.erase(dictionary);
	}
	
	ScoreComponent &Add(const ScoreComponent &scoreComponent)
	{
		const Dictionary *dictionary = scoreComponent.GetDictionary();
		assert( dictionary != NULL && m_scores.find(dictionary) == m_scores.end());
		return m_scores[dictionary] = scoreComponent;
	}

	ScoreComponent &Add(const Dictionary *dictionary)
	{
		return Add(ScoreComponent(dictionary));
	}

	void Combine(const ScoreComponentCollection &otherComponentCollection);
	std::vector<const ScoreComponent*> SortForNBestOutput() const;
};

inline std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &scoreComponentColl)
{
	ScoreComponentCollection::const_iterator iter;
	for (iter = scoreComponentColl.m_scores.begin() ; iter != scoreComponentColl.m_scores.end() ; ++iter)
	{
		const ScoreComponent &scoreComponent = iter->second;
		out << "[" << scoreComponent << "] ";
	}
	return out;
}


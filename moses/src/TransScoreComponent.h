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

//typedef float TransScoreComponent[NUM_PHRASE_SCORES];

class TransScoreComponent
{
protected:
	size_t	m_idPhraseDictionary;
	float		m_scoreComponent[NUM_PHRASE_SCORES];
public:
	TransScoreComponent()
	{
	}
	TransScoreComponent(size_t idPhraseDictionary)
	{
		m_idPhraseDictionary = idPhraseDictionary;
	}
	TransScoreComponent(const TransScoreComponent &copy)
	{
		m_idPhraseDictionary = copy.m_idPhraseDictionary;
		for (size_t i = 0 ; i < NUM_PHRASE_SCORES ; i++)
		{
			m_scoreComponent[i] = copy[i];
		}
	}

	inline size_t GetPhraseDictionaryId() const
	{
		return m_idPhraseDictionary;
	}

	float operator[](size_t index) const
	{
		return m_scoreComponent[index];
	}
	float &operator[](size_t index)
	{
		return m_scoreComponent[index];
	}

	void Reset()
	{
		for (size_t i = 0 ; i < NUM_PHRASE_SCORES ; i++)
		{
			m_scoreComponent[i] = 0;
		}
	}

	inline bool operator< (const TransScoreComponent &compare) const
	{
		return GetPhraseDictionaryId() < compare.GetPhraseDictionaryId();
	}

};

inline std::ostream& operator<<(std::ostream &out, const TransScoreComponent &transScoreComponent)
{
	out << transScoreComponent[0];
	for (size_t i = 1 ; i < NUM_PHRASE_SCORES ; i++)
	{
		out << "," << transScoreComponent[i];
	}	
	return out;
}


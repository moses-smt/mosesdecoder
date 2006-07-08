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

class PhraseDictionary;

class TransScoreComponent
{
protected:
	const PhraseDictionary *m_phraseDictionary;
	float		m_scoreComponent[NUM_PHRASE_SCORES];
public:
	TransScoreComponent()
	{ // needed by TransScoreComponentCollection
		// should try & get rid of it
	}
	TransScoreComponent(const PhraseDictionary *phraseDictionary);
	TransScoreComponent(const TransScoreComponent &copy);
	void Reset();

	size_t GetPhraseDictionaryId() const;

	float operator[](size_t index) const
	{
		return m_scoreComponent[index];
	}
	float &operator[](size_t index)
	{
		return m_scoreComponent[index];
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


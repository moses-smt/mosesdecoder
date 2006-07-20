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

class ScoreColl : public std::map<size_t, float>
{
public:
	float GetValue(size_t index) const
	{
		const_iterator iter = find(index);
		assert(iter != end());
		return iter->second;
	}
	float SetValue(size_t index, float value)
	{
		assert(find(index) != end());
		return operator[](index) = value;
	}	
	float Add(size_t index)
	{
		assert(find(index) == end());
		return operator[](index) = 0;
	}	
	void Combine(const ScoreColl &other);
};

inline std::ostream& operator<<(std::ostream &out, const ScoreColl &coll)
{
	ScoreColl::const_iterator iter;
	out << "<";
	for (iter = coll.begin() ; iter != coll.end() ; iter++)
	{
		size_t id = iter->first;
		float score = iter->second;
		out << id << "=" << score << ", ";
	}
	out << "> ";
	return out;
}


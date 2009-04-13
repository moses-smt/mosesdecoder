// $Id: DecodeGraph.h 552 2009-01-09 14:05:34Z hieu $
// vim:tabstop=2

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

#include <list>
#include <iterator>

class DecodeStep;

class DecodeGraph
{
protected:
	std::list<const DecodeStep*> m_steps;
	static size_t s_id;
	
	size_t m_id;

public:
	//! iterators
	typedef std::list<const DecodeStep*>::iterator iterator;
	typedef std::list<const DecodeStep*>::const_iterator const_iterator;
	const_iterator begin() const { return m_steps.begin(); }
	const_iterator end() const { return m_steps.end(); }
	
	DecodeGraph()
	:m_id(s_id++)
	{}

	~DecodeGraph();

	void Add(const DecodeStep *decodeStep)
	{
		m_steps.push_back(decodeStep);
	}
	
	size_t GetSize() const
	{	return m_steps.size(); }
	size_t GetId() const
	{ return m_id; }

};


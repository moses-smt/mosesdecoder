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

#include <map>
#include "HypothesisStack.h"

/** wraps up vector of HypothesisStack and takes care of adding hypotheses to
	*	the correct stack
*/
class HypothesisStackCollection
{
	typedef std::vector < HypothesisStack > StackColl;
protected:
	StackColl m_stackColl;

public:
	//! iterators
	class iterator;
	class const_iterator;
	const_iterator begin() const { return const_iterator(0, m_stackColl); }
	const_iterator end() const { return const_iterator(NOT_FOUND, m_stackColl); }

	iterator begin() { return iterator(0, m_stackColl); }
	iterator end() { return iterator(NOT_FOUND, m_stackColl); }
	const HypothesisStack &back() const { return m_stackColl.back(); }

	//! constructor
	HypothesisStackCollection(size_t sourceSize, const std::vector<const DecodeStep*> &decodeStepList)
		:m_stackColl( (size_t) pow( (float) sourceSize+1 , (int) decodeStepList.size()) )
	{}

	//! destructor
	~HypothesisStackCollection();

	//! get a particular stack
	HypothesisStack &GetStack(size_t pos)
	{
		return m_stackColl[pos];
	}

	size_t GetSize() const
	{ return m_stackColl.size(); }

	//! add hypo to appropriate stack
	void AddPrune(Hypothesis *hypo);

	// iter class
	class iterator
	{
	protected:
		size_t m_pos;
		StackColl *m_stackColl;
	public:
		iterator() {}
		iterator(size_t pos, StackColl &m_stackColl);
		bool operator!=(const iterator &compare) const;
		const iterator &operator++()
		{
			m_pos = (m_pos >= (m_stackColl->size()-1) ) ? NOT_FOUND : m_pos+1 ;
			return *this;
		}
		HypothesisStack &operator*() const
		{
			return (*m_stackColl)[m_pos];
		}
	};

	class const_iterator
	{
	protected:
		size_t m_pos;
		const StackColl *m_stackColl;
	public:
		const_iterator() {}
		const_iterator(size_t pos, const StackColl &m_stackColl);
		bool operator!=(const const_iterator &compare) const;
		const const_iterator &operator++()
		{
			m_pos = (m_pos >= (m_stackColl->size()-1) ) ? NOT_FOUND : m_pos+1 ;
			return *this;
		}
		const HypothesisStack &operator*() const
		{
			return (*m_stackColl)[m_pos];
		}
	};
};


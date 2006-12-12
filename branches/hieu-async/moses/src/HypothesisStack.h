// $Id: HypothesisStack.h 1051 2006-12-06 22:23:52Z hieuhoang1972 $

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
#include "HypothesisCollection.h"

/** wraps up vector of HypothesisCollection and takes care of adding hypotheses to
	*	the correct stack
*/
class HypothesisStack
{
	typedef std::vector < HypothesisCollection > StackType;
protected:
	StackType m_stack;

public:
	//! iterators
	class iterator;
	class const_iterator;
	iterator begin() { return iterator(0, m_stack); }
	iterator end() { return iterator(NOT_FOUND, m_stack); }
	const HypothesisCollection &back() const { return m_stack.back(); }

	//! constructor
	HypothesisStack(size_t sourceSize, const std::vector<DecodeStep*> &decodeStepList)
		:m_stack( (size_t) pow( (float) sourceSize+1 , (int) decodeStepList.size()) )
	{}

	//! get a particular stack
	HypothesisCollection &GetStack(size_t pos)
	{
		return m_stack[pos];
	}

	//! add hypo to appropriate stack
	void AddPrune(Hypothesis *hypo);

	// iter class
	class iterator
	{
	protected:
		size_t m_pos;
		StackType *m_stack;
	public:
		iterator() {}
		iterator(size_t pos, StackType &m_stack);
		bool operator!=(const iterator &compare) const;
		const iterator &operator++()
		{
			m_pos = (m_pos >= (m_stack->size()-1) ) ? NOT_FOUND : m_pos+1 ;
			return *this;
		}
		HypothesisCollection &operator*() const
		{
			return (*m_stack)[m_pos];
		}
	};

};


// $Id: HypothesisStackCollection.h 215 2007-11-20 17:25:55Z hieu $

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
#include "DecodeStepCollection.h"

/** wraps up vector of HypothesisStack and takes care of adding hypotheses to
	*	the correct stack
*/
class HypothesisStackCollection
{
	typedef std::vector < HypothesisStack > StackColl;
	friend std::ostream& operator<<(std::ostream&, const HypothesisStackCollection&);
protected:
	size_t 		m_sourceSize;
	StackColl	m_stackColl;

public:
	//! iterators
	class iterator;
	class const_iterator;
	const_iterator begin() const { return const_iterator(0, m_stackColl); }
	const_iterator end() const { return const_iterator(NOT_FOUND, m_stackColl); }

	iterator begin() { return iterator(0, m_stackColl); }
	iterator end() { return iterator(NOT_FOUND, m_stackColl); }
	
	//! constructor
	HypothesisStackCollection(size_t sourceSize, const DecodeStepCollection &decodeStepColl
													, size_t maxHypoStackSize, float beamThreshold);

	//! destructor
	~HypothesisStackCollection();

	//! get a particular stack
	HypothesisStack &GetStack(size_t pos)
	{	return m_stackColl[pos];}
	const HypothesisStack &GetStack(size_t pos) const
	{	return m_stackColl[pos];}

	size_t GetSize() const
	{ return m_stackColl.size(); }

	//! add hypo to appropriate stack
	void AddPrune(Hypothesis *hypo);

	//! return last stack, or 1st if last stack is empty.
	const HypothesisStack &GetOutputStack() const;
	
	// logging
	void OutputHypoStackSize(bool formatted, size_t sourceSize) const;
	/** Output arc list information to debug mem leak */
	void OutputArcListSize() const;

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


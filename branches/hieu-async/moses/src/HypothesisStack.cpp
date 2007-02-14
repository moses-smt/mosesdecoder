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

#include "HypothesisStack.h"
#include "StaticData.h"

HypothesisStack::iterator::iterator(size_t pos, StackType &stack)
{
	m_pos = pos;
	m_stack = &stack;
}

bool HypothesisStack::iterator::operator!=(const iterator &compare) const
{
	return this->m_pos != compare.m_pos;
}

HypothesisStack::const_iterator::const_iterator(size_t pos, const StackType &stack)
{
	m_pos = pos;
	m_stack = &stack;
}

bool HypothesisStack::const_iterator::operator!=(const const_iterator &compare) const
{
	return this->m_pos != compare.m_pos;
}

void HypothesisStack::AddPrune(Hypothesis *hypo)
{
	const WordsBitmap &wordsBitmap = hypo->GetSourceBitmap();
	size_t stackIndex = wordsBitmap.GetStackIndex();

	if (stackIndex == m_stack.size() - 1)
	{	// only add to last stack if all factors are specified, ie. sync
		if (!hypo->GetTargetPhrase().IsSynchronized())
		{
		FREEHYPO(hypo);		
		return;
		}
	}
	else
	{ // only add to stack if no dangly words which can never be fitted with other factors
		/*
		if (!hypo->GetAl)
		{
		FREEHYPO(hypo);		
		return;
		}
		*/
	}
	m_stack[stackIndex].AddPrune(hypo);
}


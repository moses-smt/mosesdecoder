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

#include "HypothesisStackCollection.h"
#include "StaticData.h"

HypothesisStackCollection::~HypothesisStackCollection()
{
	StackColl::reverse_iterator iter;
	for (iter = m_stackColl.rbegin() ; iter != m_stackColl.rend() ; ++iter)
	{
		HypothesisStack &hypoColl = *iter;
		hypoColl.RemoveAll();
	}
}

HypothesisStackCollection::iterator::iterator(size_t pos, StackColl &stackColl)
{
	m_pos = pos;
	m_stackColl = &stackColl;
}

bool HypothesisStackCollection::iterator::operator!=(const iterator &compare) const
{
	return this->m_pos != compare.m_pos;
}

HypothesisStackCollection::const_iterator::const_iterator(size_t pos, const StackColl &stackColl)
{
	m_pos = pos;
	m_stackColl = &stackColl;
}

bool HypothesisStackCollection::const_iterator::operator!=(const const_iterator &compare) const
{
	return this->m_pos != compare.m_pos;
}

void HypothesisStackCollection::AddPrune(Hypothesis *hypo)
{
	const WordsBitmap &wordsBitmap = hypo->GetSourceBitmap();
	size_t stackIndex = wordsBitmap.GetStackIndex();

	if (stackIndex == m_stackColl.size() - 1)
	{	// only add to last stack if all factors are specified, ie. sync
		if (!hypo->IsSynchronized())
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
	m_stackColl[stackIndex].AddPrune(hypo);
}


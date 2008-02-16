// $Id: HypothesisStackCollection.cpp 220 2007-11-21 16:06:46Z hieu $

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

//! constructor
HypothesisStackCollection::HypothesisStackCollection
			(size_t sourceSize, const DecodeStepCollection &decodeStepColl
			,size_t maxHypoStackSize, float beamThreshold)
:m_sourceSize(sourceSize)
,m_stackColl( (size_t) pow( (float) sourceSize+1 , (int) decodeStepColl.GetSize()) )
{
	const StaticData &staticData = StaticData::Instance();
	bool biggerNonDiagHypoStack;

	AsyncMethod asyncMethod = staticData.GetAsyncMethod();
	switch (asyncMethod)
	{
	case UpperDiagonal:
	case NonTiling:
	case MultipleFirstStep:
		biggerNonDiagHypoStack = true;
		break;
	default:
		biggerNonDiagHypoStack = false;
		break;
	}
	size_t nonDiagStackSize = staticData.GetNonDiagStackSize();

	size_t count = 0;
	size_t stackNo = 0;
	StackColl::iterator iterStack;
	for (iterStack = m_stackColl.begin() ; iterStack != m_stackColl.end() ; ++iterStack)
	{
		HypothesisStack &hypoStack = *iterStack;
		if (asyncMethod == MultipassLarge1st)
		{
			if (stackNo <= sourceSize)
				hypoStack.SetMaxHypoStackSize(nonDiagStackSize);
			else
				hypoStack.SetMaxHypoStackSize(maxHypoStackSize);
		}
		else if (asyncMethod == MultipassLargeLast)
		{
			const size_t lastStackIndex = (sourceSize+1) ^ decodeStepColl.GetSize() - 1;
			if ((stackNo + 1) % (sourceSize + 1) == 0 
					&& stackNo != sourceSize 
					&& stackNo != lastStackIndex )
			{
				hypoStack.SetMaxHypoStackSize(nonDiagStackSize);
			}
			else
			{
				hypoStack.SetMaxHypoStackSize(maxHypoStackSize);
			}
		}
		else if (!biggerNonDiagHypoStack || count == 0)
		{
			hypoStack.SetMaxHypoStackSize(maxHypoStackSize);
			hypoStack.SetBeamThreshold(beamThreshold);
			count = sourceSize+1;
		}
		else
		{
			hypoStack.SetMaxHypoStackSize(nonDiagStackSize);
			count--;
		}

		stackNo++;
	}
}

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

const HypothesisStack &HypothesisStackCollection::GetOutputStack() const
{
	AsyncMethod asyncMethod = StaticData::Instance().GetAsyncMethod();
	
	const HypothesisStack &lastStack = m_stackColl.back();
	if (lastStack.GetSize() > 0)
		return lastStack;
	else if (asyncMethod == MultipassLargeLast 
					&& m_stackColl[m_sourceSize].GetSize() > 0)
		return m_stackColl[m_sourceSize];
	else
		return m_stackColl.front();
}

/**
 * Logging of hypothesis stack sizes
 */
 // helper
void OutputNumber(size_t num)
{
	if (num > 99)
		TRACE_ERR(" ");
	else if (num > 9)
		TRACE_ERR("  ");
	else
		TRACE_ERR("   ");
	TRACE_ERR(num);
}

void HypothesisStackCollection::OutputHypoStackSize(bool formatted, size_t sourceSize) const
{
	const_iterator iterStack;
	TRACE_ERR( "Stack sizes: ");
	
	if (formatted)
		TRACE_ERR(endl);
	
	int sqSize	= (int) sourceSize + 1
			, i 		= 1;
	for (iterStack = begin() ; iterStack != end() ; ++iterStack)
	{
		if (formatted)
		{
			OutputNumber((*iterStack).size());
			if (i++ % sqSize == 0)
				TRACE_ERR( endl);
		}
		else
			OutputNumber((*iterStack).size());
	}
	
	if (!formatted)
		TRACE_ERR(endl);
}

void HypothesisStackCollection::OutputArcListSize() const
{
	TRACE_ERR( "Arc sizes: ");
	
	HypothesisStackCollection::const_iterator iterStack;
	for (iterStack = begin() ; iterStack != end() ; ++iterStack)
	{
		const HypothesisStack &hypoColl = *iterStack;
		
		size_t arcCount = 0;
		HypothesisStack::const_iterator iterColl;
		for (iterColl = hypoColl.begin() ; iterColl != hypoColl.end() ; ++iterColl)
		{
			Hypothesis *hypo = *iterColl;
			const ArcList *arcList = hypo->GetArcList();
			if (arcList != NULL)
				arcCount += arcList->size();
		}
		
		TRACE_ERR( ", " << arcCount);
	}
	TRACE_ERR(endl);
}

ostream& operator<<(ostream& out, const HypothesisStackCollection &coll)
{	
	int i = 0;
	HypothesisStackCollection::const_iterator iterStack;
	for (iterStack = coll.begin() ; iterStack != coll.end() ; ++iterStack)
	{
		const HypothesisStack &hypoColl = *iterStack;
		if (hypoColl.GetSize() > 0)
			out << "Stack " << i++ << ": " << endl << hypoColl << endl;
	}
	return out;
}

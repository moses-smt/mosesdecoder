// $Id: PartialTranslOptColl.cpp 168 2007-10-26 16:05:15Z hieu $

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

#include <algorithm>
#include "PartialTranslOptColl.h"

/** constructor, intializes counters and thresholds */
PartialTranslOptColl::PartialTranslOptColl()
{
}

/** add a partial translation option to the collection, prune if necessary.
 * This is done similar to the Prune() in TranslationOptionCollection */ 

void PartialTranslOptColl::Add(TranslationOption *partialTranslOpt)
{
	// add
	partialTranslOpt->CalcScore();
	m_list.push_back(partialTranslOpt);
}

void PartialTranslOptColl::Add(const PartialTranslOptColl &other)
{
	const_iterator iterTransOptColl;
	for (iterTransOptColl = other.begin() ; iterTransOptColl != other.end() ; ++iterTransOptColl)
	{
		TranslationOption *transOpt = *iterTransOptColl;
		Add(transOpt);
	}
}

/** helper, used by pruning */
bool ComparePartialTranslationOption(const TranslationOption *a, const TranslationOption *b)
{
	return a->GetFutureScore() > b->GetFutureScore();
}

std::ostream& operator<<(std::ostream& out, const PartialTranslOptColl &coll)
{
	PartialTranslOptColl::const_iterator iter;
	
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		const TranslationOption &transOpt = **iter;
		out << transOpt << endl;
		
	}
	return out;
}

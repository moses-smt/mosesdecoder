// $Id: AlignmentElement.cpp 988 2006-11-21 19:35:37Z hieuhoang1972 $
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
#include "AlignmentElement.h"

using namespace std;

AlignmentElement::AlignmentElement(const vector<size_t> &alignInfo)
{
	insert_iterator<ContainerType> insertIter( m_collection, m_collection.end() );
	copy(alignInfo.begin(), alignInfo.end(), insertIter);
}

void AlignmentElement::Shift(int shift)
{
	ContainerType  newColl;

	ContainerType::const_iterator iter;
	for (iter = m_collection.begin() ; iter != m_collection.end() ; ++iter)
		newColl.insert( *iter + shift);

	m_collection = newColl;
}

std::ostream& operator<<(std::ostream& out, const AlignmentElement &alignElement)
{
	AlignmentElement::ContainerType::const_iterator iter;
	for (iter = alignElement.GetCollection().begin() ; iter != alignElement.GetCollection().end() ; ++iter)
		out << *iter;

	return out;
}


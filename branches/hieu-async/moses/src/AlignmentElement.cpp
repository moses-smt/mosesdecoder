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

AlignmentElement::AlignmentElement(const std::vector<size_t> &copy)
{
	std::copy(copy.begin(), copy.end(), back_insert_iterator< std::vector<size_t> > (m_collection));
}

void AlignmentElement::Shift(int shift)
{
	for (size_t index = 0 ; index < GetSize() ; ++index)
		m_collection[index] += shift;
}

std::ostream& operator<<(std::ostream& out, const AlignmentElement &alignElement)
{
	for (size_t index = 0 ; index < alignElement.GetSize() ; ++index)
		out << alignElement.m_collection[index];

	return out;
}


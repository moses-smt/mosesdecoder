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

#include <algorithm>
#include <iostream>
#include "AlignmentPhrase.h"

using namespace std;

namespace MosesTraining
{

void AlignmentElement::Merge(size_t align)
{
  m_elements.insert(align);
}

void AlignmentPhrase::Merge(const std::vector< std::vector<size_t> > &source)
{
  for (size_t idx = 0 ; idx < source.size() ; ++idx) {
    AlignmentElement &currElement = m_elements[idx];
    const vector<size_t> &newElement = source[idx];

    for (size_t pos = 0 ; pos < newElement.size() ; ++pos) {
      currElement.Merge(newElement[pos]);
    }
  }
}

} // namespace



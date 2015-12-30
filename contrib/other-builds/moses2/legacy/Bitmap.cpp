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

#include <boost/functional/hash.hpp>
#include "Bitmap.h"
#include "../MemPool.h"

using namespace std;

namespace Moses2
{

Bitmap::Bitmap(size_t size, const std::vector<bool>& initializer, MemPool &pool)
:m_size(size)
{
  //m_bitmap = (char*) calloc(size, sizeof(char));
  m_bitmap = (bool*) pool.AllocatePadded(size);
  Init<bool>(m_bitmap, size, false);

  // The initializer may not be of the same length.  Change to the desired
  // length.  If we need to add any elements, initialize them to false.
  for (size_t i = 0; i < initializer.size(); ++i) {
	  m_bitmap[i] = initializer[i];
  }

  m_numWordsCovered = std::count(initializer.begin(), initializer.end(), true);

  // Find the first gap, and cache it.
  m_firstGap = NOT_FOUND;
  for (size_t i = 0; i < size; ++i) {
	  if (!m_bitmap[i]) {
		  m_firstGap = i;
		  break;
	  }
  }
}

Bitmap::Bitmap(const Bitmap &copy, const Range &range, MemPool &pool)
:m_size(copy.m_size)
,m_firstGap(copy.m_firstGap)
,m_numWordsCovered(copy.m_numWordsCovered)
{
  //m_bitmap = (char*) malloc(m_size);
  m_bitmap = (bool*) pool.AllocatePadded(m_size);
  memcpy(m_bitmap, copy.m_bitmap, m_size);

  SetValueNonOverlap(range);
}

Bitmap::~Bitmap()
{
  //free(m_bitmap);
}

// for unordered_set in stack
size_t Bitmap::hash() const
{
  size_t ret = boost::hash_value(m_bitmap);
  return ret;
}

bool Bitmap::operator==(const Bitmap& other) const
{
  return m_bitmap == other.m_bitmap;
}

// friend
std::ostream& operator<<(std::ostream& out, const Bitmap& bitmap)
{
  for (size_t i = 0 ; i < bitmap.m_size ; i++) {
    out << int(bitmap.GetValue(i));
  }
  return out;
}

}


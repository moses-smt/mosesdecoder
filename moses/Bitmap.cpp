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

namespace Moses
{

TO_STRING_BODY(Bitmap);

Bitmap::Bitmap(size_t size, const std::vector<bool>& initializer)
  :m_bitmap(initializer.begin(), initializer.end())
{

  // The initializer may not be of the same length.  Change to the desired
  // length.  If we need to add any elements, initialize them to false.
  m_bitmap.resize(size, false);

  m_numWordsCovered = std::count(m_bitmap.begin(), m_bitmap.end(), true);

  // Find the first gap, and cache it.
  std::vector<char>::const_iterator first_gap = std::find(
        m_bitmap.begin(), m_bitmap.end(), false);
  m_firstGap = (
                 (first_gap == m_bitmap.end()) ?
                 NOT_FOUND : first_gap - m_bitmap.begin());
}

//! Create Bitmap of length size and initialise.
Bitmap::Bitmap(size_t size)
  :m_bitmap(size, false)
  ,m_firstGap(0)
  ,m_numWordsCovered(0)

{
}

//! Deep copy.
Bitmap::Bitmap(const Bitmap &copy)
  :m_bitmap(copy.m_bitmap)
  ,m_firstGap(copy.m_firstGap)
  ,m_numWordsCovered(copy.m_numWordsCovered)
{
}

Bitmap::Bitmap(const Bitmap &copy, const Range &range)
  :m_bitmap(copy.m_bitmap)
  ,m_firstGap(copy.m_firstGap)
  ,m_numWordsCovered(copy.m_numWordsCovered)
{
  SetValueNonOverlap(range);
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
  for (size_t i = 0 ; i < bitmap.m_bitmap.size() ; i++) {
    out << int(bitmap.GetValue(i));
  }
  return out;
}

} // namespace



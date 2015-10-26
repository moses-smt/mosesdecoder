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

Bitmap::Bitmap(const Bitmap &copy, const Range &range)
:m_bitmap(copy.m_bitmap)
,m_firstGap(copy.m_firstGap)
{
	SetValue(range, true);
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



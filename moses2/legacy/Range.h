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

#pragma once

#include <cassert>
#include <iostream>
#include <boost/functional/hash.hpp>
#include "Util2.h"
#include "util/exception.hh"

#ifdef WIN32
#undef max
#endif

namespace Moses2
{

/***
 * Efficient version of Bitmap for contiguous ranges
 */
class Range
{
  friend std::ostream& operator <<(std::ostream& out, const Range& range);

  // m_endPos is inclusive
  size_t m_startPos, m_endPos;
public:
  inline explicit Range() {
  }
  inline Range(size_t startPos, size_t endPos) :
    m_startPos(startPos), m_endPos(endPos) {
  }
  inline Range(const Range &copy) :
    m_startPos(copy.GetStartPos()), m_endPos(copy.GetEndPos()) {
  }

  inline size_t GetStartPos() const {
    return m_startPos;
  }
  inline size_t GetEndPos() const {
    return m_endPos;
  }

  inline void SetStartPos(size_t val) {
    m_startPos = val;
  }
  inline void SetEndPos(size_t val) {
    m_endPos = val;
  }

  //! count of words translated
  inline size_t GetNumWordsCovered() const {
    assert(
      (m_startPos == NOT_FOUND && m_endPos == NOT_FOUND) || (m_startPos != NOT_FOUND && m_endPos != NOT_FOUND));
    return (m_startPos == NOT_FOUND) ? 0 : m_endPos - m_startPos + 1;
  }

  //! transitive comparison
  inline bool operator<(const Range& x) const {
    return (m_startPos<x.m_startPos
            || (m_startPos==x.m_startPos && m_endPos<x.m_endPos));
  }

  // equality operator
  inline bool operator==(const Range& x) const {
    return (m_startPos==x.m_startPos && m_endPos==x.m_endPos);
  }
  // Whether two word ranges overlap or not
  inline bool Overlap(const Range& x) const {

    if ( x.m_endPos < m_startPos || x.m_startPos > m_endPos) return false;

    return true;
  }

  inline size_t GetNumWordsBetween(const Range& x) const {
    UTIL_THROW_IF2(Overlap(x), "Overlapping ranges");

    if (x.m_endPos < m_startPos) {
      return m_startPos - x.m_endPos - 1;
    }

    return x.m_startPos - m_endPos - 1;
  }

};

inline size_t hash_value(const Range& range)
{
  size_t seed = range.GetStartPos();
  boost::hash_combine(seed, range.GetEndPos());
  return seed;
}

}


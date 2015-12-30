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

#include <algorithm>
#include <limits>
#include <vector>
#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include "Range.h"

namespace Moses2
{
class MemPool;

typedef unsigned long WordsBitmapID;

/** Vector of boolean to represent whether a word has been translated or not.
 *
 * Implemented using a vector of char, which is usually the same representation
 * for the elements that a C array of bool would use.  A vector of bool, or a
 * Boost dynamic_bitset, could be much more efficient in theory.  Unfortunately
 * algorithms like std::find() are not optimized for vector<bool> on gcc or
 * clang, and dynamic_bitset lacks all the optimized search operations we want.
 * Only benchmarking will tell what works best.  Perhaps dynamic_bitset could
 * still be a dramatic improvement, if we flip the meaning of the bits around
 * so we can use its find_first() and find_next() for the most common searches.
 */
class Bitmap
{
  friend std::ostream& operator<<(std::ostream& out, const Bitmap& bitmap);
private:
  size_t m_size;
  char *m_bitmap; //! Ticks of words in sentence that have been done.
  size_t m_firstGap; //! Cached position of first gap, or NOT_FOUND.
  size_t m_numWordsCovered;

  Bitmap(); // not implemented
  Bitmap& operator= (const Bitmap& other);

  /** Update the first gap, when bits are flipped */
  void UpdateFirstGap(size_t startPos, size_t endPos, bool value) {
    if (value) {
      //may remove gap
      if (startPos <= m_firstGap && m_firstGap <= endPos) {
        m_firstGap = NOT_FOUND;
        for (size_t i = endPos + 1 ; i < m_size; ++i) {
          if (!m_bitmap[i]) {
            m_firstGap = i;
            break;
          }
        }
      }

    } else {
      //setting positions to false, may add new gap
      if (startPos < m_firstGap) {
        m_firstGap = startPos;
      }
    }
  }

  //! set value between 2 positions, inclusive
  void
  SetValueNonOverlap(Range const& range) {
    size_t startPos = range.GetStartPos();
    size_t endPos = range.GetEndPos();

    for(size_t pos = startPos ; pos <= endPos ; pos++) {
      m_bitmap[pos] = true;
    }

    m_numWordsCovered += range.GetNumWordsCovered();
    UpdateFirstGap(startPos, endPos, true);
  }

public:
  //! Create Bitmap of length size, and initialise with vector.
  explicit Bitmap(size_t size, const std::vector<bool>& initializer, MemPool &pool);
  explicit Bitmap(const Bitmap &copy, const Range &range, MemPool &pool);
  ~Bitmap();

  //! Count of words translated.
  size_t GetNumWordsCovered() const {
    return m_numWordsCovered;
  }

  //! position of 1st word not yet translated, or NOT_FOUND if everything already translated
  size_t GetFirstGapPos() const {
    return m_firstGap;
  }


  //! position of last word not yet translated, or NOT_FOUND if everything already translated
  size_t GetLastGapPos() const {
    for (int pos = int(m_size) - 1 ; pos >= 0 ; pos--) {
      if (!m_bitmap[pos]) {
        return pos;
      }
    }
    // no starting pos
    return NOT_FOUND;
  }


  //! position of last translated word
  size_t GetLastPos() const {
    for (int pos = int(m_size) - 1 ; pos >= 0 ; pos--) {
      if (m_bitmap[pos]) {
        return pos;
      }
    }
    // no starting pos
    return NOT_FOUND;
  }

  //! whether a word has been translated at a particular position
  bool GetValue(size_t pos) const {
    return bool(m_bitmap[pos]);
  }
  //! set value at a particular position
  void SetValue( size_t pos, bool value ) {
    bool origValue = m_bitmap[pos];
    if (origValue == value) {
      // do nothing
    } else {
      m_bitmap[pos] = value;
      UpdateFirstGap(pos, pos, value);
      if (value) {
        ++m_numWordsCovered;
      } else {
        --m_numWordsCovered;
      }
    }
  }

  //! whether every word has been translated
  bool IsComplete() const {
    return GetSize() == GetNumWordsCovered();
  }
  //! whether the wordrange overlaps with any translated word in this bitmap
  bool Overlap(const Range &compare) const {
    for (size_t pos = compare.GetStartPos() ; pos <= compare.GetEndPos() ; pos++) {
      if (m_bitmap[pos])
        return true;
    }
    return false;
  }
  //! number of elements
  size_t GetSize() const {
    return m_size;
  }

  inline size_t GetEdgeToTheLeftOf(size_t l) const {
    if (l == 0) return l;
    while (l && !m_bitmap[l-1]) {
      --l;
    }
    return l;
  }

  inline size_t GetEdgeToTheRightOf(size_t r) const {
    if (r+1 == m_size) return r;

    for (size_t i = r + 1; i < m_size; ++i) {
    	if (m_bitmap[i]) {
    		return i - 1;
    	}
    }
    return NOT_FOUND;
  }

  // for unordered_set in stack
  size_t hash() const;
  bool operator==(const Bitmap& other) const;
  bool operator!=(const Bitmap& other) const {
    return !(*this == other);
  }

};

}

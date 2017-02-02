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

#include <ostream>
#include <string>
#include <vector>
#include "util/string_piece.hh"

namespace Moses2
{

struct FactorFriend;
class FactorCollection;

/** Represents a factor (word, POS, etc).
 * A Factor has a contiguous identifier and string value.
 */
class Factor
{
  friend std::ostream& operator<<(std::ostream&, const Factor&);

  // only these classes are allowed to instantiate this class
  friend class FactorCollection;
  friend struct FactorFriend;

  // FactorCollection writes here.
  // This is mutable so the pointer can be changed to pool-backed memory.
  mutable StringPiece m_string;
  size_t m_id;

  //! protected constructor. only friend class, FactorCollection, is allowed to create Factor objects
  Factor() {
  }

  // Needed for STL containers.  They'll delegate through FactorFriend, which is never exposed publicly.
  Factor(const Factor &factor) :
    m_string(factor.m_string), m_id(factor.m_id) {
  }

  // Not implemented.  Shouldn't be called.
  Factor &operator=(const Factor &factor);

public:
  //! original string representation of the factor
  StringPiece GetString() const {
    return m_string;
  }
  //! contiguous ID
  inline size_t GetId() const {
    return m_id;
  }

  /** transitive comparison between 2 factors.
   *	-1 = less than
   *	+1 = more than
   *	0	= same
   */
  inline int Compare(const Factor &compare) const {
    if (this < &compare) return -1;
    if (this > &compare) return 1;
    return 0;
  }
  //! transitive comparison used for adding objects into FactorCollection
  inline bool operator<(const Factor &compare) const {
    return this < &compare;
  }

  // quick equality comparison. Not used
  inline bool operator==(const Factor &compare) const {
    return this == &compare;
  }
};

size_t hash_value(const Factor &f);

}


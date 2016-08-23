/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2011 University of Edinburgh

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

#include "Factor.h"
#include "Word.h"

#include <iostream>
#include <boost/functional/hash.hpp>
#include <boost/unordered_set.hpp>

#include <set>

namespace Moses
{

/** Functor to create hash for words.
 * @todo uses all factors, not just factor 0
 */
class NonTerminalHasher
{
public:
  size_t operator()(const Word & k) const {
    // Assumes that only the first factor is relevant.
    const Factor * f = k[0];
    return hash_value(*f);
  }
};

/** Functor to test whether 2 words are the same
 * @todo uses all factors, not just factor 0
 */
class NonTerminalEqualityPred
{
public:
  bool operator()(const Word & k1, const Word & k2) const {
    // Assumes that only the first factor is relevant.
    const Factor * f1 = k1[0];
    const Factor * f2 = k2[0];
    return !(f1->Compare(*f2));
  }
};

typedef boost::unordered_set<Word,
        NonTerminalHasher,
        NonTerminalEqualityPred> NonTerminalSet;

std::ostream& operator<<(std::ostream&, const NonTerminalSet&);

}  // namespace Moses

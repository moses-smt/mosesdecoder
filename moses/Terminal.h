/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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

#include "TypeDef.h"
#include "Word.h"
#include "Factor.h"

#include <boost/functional/hash.hpp>

namespace Moses
{

class TerminalHasher
{
public:
  // Generate a hash value for a word representing a terminal.  It's
  // assumed that the same subset of factors will be active for all words
  // that are hashed.
  size_t operator()(const Word &t) const {
    size_t seed = 0;
    for (size_t i = 0; i < MAX_NUM_FACTORS; ++i) {
      const Factor *f = t[i];
      if (f) {
        boost::hash_combine(seed, *f);
      }
    }
    return seed;
  }
};

class TerminalEqualityPred
{
public:
  // Equality predicate for comparing words representing terminals.  As
  // with the hasher, it's assumed that all words will have the same
  // subset of active factors.
  bool operator()(const Word &t1, const Word &t2) const {
    for (size_t i = 0; i < MAX_NUM_FACTORS; ++i) {
      const Factor *f1 = t1[i];
      const Factor *f2 = t2[i];
      if (f1 && f1->Compare(*f2)) {
        return false;
      }
    }
    return true;
  }
};

}  // namespace Moses

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

#include "FactorTypeSet.h"

using namespace std;

namespace Moses
{
FactorMask::FactorMask(const vector<FactorType> &factors)
{
  vector<FactorType>::const_iterator iter;
  for (iter = factors.begin() ; iter != factors.end() ; ++iter) {
    this->set(*iter);
  }
}

bool FactorMask::IsUseable(const FactorMask &other) const
{
  for (size_t i = 0; i < other.size(); ++i) {
    if (other[i]) {
      if (!this->operator[](i) ) {
        return false;
      }
    }
  }

  return true;
}

TO_STRING_BODY(FactorMask);

// friend
std::ostream& operator<<(std::ostream& out, const FactorMask& fm)
{
  out << "FactorMask<";
  bool first = true;
  for (size_t currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
    if (fm[currFactor]) {
      if (first) {
        first = false;
      } else {
        out << ",";
      }
      out << currFactor;
    }
  }
  out << ">";

  return out;
}

}



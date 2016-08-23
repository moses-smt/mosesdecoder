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

#ifndef moses_FactorTypeSet_h
#define moses_FactorTypeSet_h

#include <iostream>
#include <bitset>
#include <vector>
#include "TypeDef.h"
#include "Util.h"

namespace Moses
{

/** set of unique FactorTypes. Used to store what factor types are used in phrase tables etc
 */
class FactorMask : public std::bitset<MAX_NUM_FACTORS>
{
  friend std::ostream& operator<<(std::ostream&, const FactorMask&);

public:
  //! construct object from list of FactorType.
  explicit FactorMask(const std::vector<FactorType> &factors);
  //! default constructor
  inline FactorMask() {}
  //! copy constructor
  FactorMask(const std::bitset<MAX_NUM_FACTORS>& rhs) : std::bitset<MAX_NUM_FACTORS>(rhs) { }

  bool IsUseable(const FactorMask &other) const;

  TO_STRING();
};

}
#endif

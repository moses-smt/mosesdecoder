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

#include <iostream>
#include "legacy/Util2.h"
#include "legacy/Bitmap.h"
#include "legacy/Matrix.h"

namespace Moses2
{
class MemPool;
class System;

//! A square array of floats to store future costs in the phrase-based decoder
class EstimatedScores: public Matrix<float>
{
public:
  EstimatedScores(MemPool &pool, size_t size) :
    Matrix<float>(pool, size, size) {
  }

  ~EstimatedScores(); // not implemented

  float CalcEstimatedScore(Bitmap const&) const;
  float CalcEstimatedScore(Bitmap const&, size_t startPos, size_t endPos) const;

  std::ostream &Debug(std::ostream &out, const System &system) const {
    for (size_t endPos = 0; endPos < GetSize(); endPos++) {
      for (size_t startPos = 0; startPos < GetSize(); startPos++)
        out << GetValue(startPos, endPos) << " ";
      out << std::endl;
    }
    return out;
  }

};

}


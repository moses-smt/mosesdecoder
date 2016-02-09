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
#include "legacy/SquareMatrix.h"

namespace Moses2
{
//! A square array of floats to store future costs in the phrase-based decoder
class EstimatedScores : public SquareMatrix<float>
{
  friend std::ostream& operator<<(std::ostream &out, const EstimatedScores &matrix);

public:
  EstimatedScores(size_t size)
  :SquareMatrix(size, size)
  {}

  float CalcEstimatedScore( Bitmap const& ) const;
  float CalcEstimatedScore( Bitmap const&, size_t startPos, size_t endPos ) const;

};

inline std::ostream& operator<<(std::ostream &out, const EstimatedScores &matrix)
{
  for (size_t endPos = 0 ; endPos < matrix.GetSize() ; endPos++) {
    for (size_t startPos = 0 ; startPos < matrix.GetSize() ; startPos++)
      out << matrix.GetValue(startPos, endPos) << " ";
    out << std::endl;
  }

  return out;
}

}


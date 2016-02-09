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
#include "Util2.h"
#include "Bitmap.h"

namespace Moses2
{

class SquareMatrix
{
  friend std::ostream& operator<<(std::ostream &out, const SquareMatrix &matrix);
protected:
  const size_t m_size; /**< length of the square (sentence length) */
  float *m_array; /**< two-dimensional array to store floats */

  SquareMatrix(); // not implemented
  SquareMatrix(const SquareMatrix &copy); // not implemented

public:
  SquareMatrix(size_t size)
    :m_size(size) {
    m_array = (float*) malloc(sizeof(float) * size * size);
  }
  ~SquareMatrix() {
    free(m_array);
  }

  // set upper triangle
  void InitTriangle(const float &val)
  {
    for(size_t row=0; row < m_size; row++) {
      for(size_t col=row; col<m_size; col++) {
        SetScore(row, col, val);
      }
    }
  }

  /** Returns length of the square: typically the sentence length */
  inline size_t GetSize() const {
    return m_size;
  }
  /** Get a future cost score for a span */
  inline const float &GetScore(size_t startPos, size_t endPos) const {
    return m_array[startPos * m_size + endPos];
  }
  /** Set a future cost score for a span */
  inline void SetScore(size_t startPos, size_t endPos, const float &value) {
    m_array[startPos * m_size + endPos] = value;
  }
};

inline std::ostream& operator<<(std::ostream &out, const SquareMatrix &matrix)
{
  for (size_t endPos = 0 ; endPos < matrix.GetSize() ; endPos++) {
    for (size_t startPos = 0 ; startPos < matrix.GetSize() ; startPos++)
      out << matrix.GetScore(startPos, endPos) << " ";
    out << std::endl;
  }

  return out;
}

/////////////////////////////////////////////////////////////////////////////////////
//! A square array of floats to store future costs in the phrase-based decoder
class EstimatedScores : public SquareMatrix
{
public:
  EstimatedScores(size_t size)
  :SquareMatrix(size)
  {}

  float CalcEstimatedScore( Bitmap const& ) const;
  float CalcEstimatedScore( Bitmap const&, size_t startPos, size_t endPos ) const;

};

}


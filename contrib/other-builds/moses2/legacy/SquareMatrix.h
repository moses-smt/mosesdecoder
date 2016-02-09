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

namespace Moses2
{
template<typename T>
class SquareMatrix
{
protected:
  const size_t m_size; /**< length of the square (sentence length) */
  T *m_array; /**< two-dimensional array to store floats */

  SquareMatrix(); // not implemented
  SquareMatrix(const SquareMatrix &copy); // not implemented

public:
  SquareMatrix(size_t size)
    :m_size(size) {
    m_array = (T*) malloc(sizeof(T) * size * size);
  }
  ~SquareMatrix() {
    free(m_array);
  }

  // set upper triangle
  void InitTriangle(const T &val)
  {
    for(size_t row=0; row < m_size; row++) {
      for(size_t col=row; col<m_size; col++) {
    	  SetValue(row, col, val);
      }
    }
  }

  /** Returns length of the square: typically the sentence length */
  inline size_t GetSize() const {
    return m_size;
  }
  /** Get a future cost score for a span */
  inline const T &GetValue(size_t startPos, size_t endPos) const {
    return m_array[startPos * m_size + endPos];
  }
  /** Set a future cost score for a span */
  inline void SetValue(size_t startPos, size_t endPos, const T &value) {
    m_array[startPos * m_size + endPos] = value;
  }
};


}


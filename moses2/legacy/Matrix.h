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
#include "../MemPool.h"

namespace Moses2
{
template<typename T>
class Matrix
{
protected:
  size_t m_rows, m_cols; /**< length of the square (sentence length) */
  T *m_array; /**< two-dimensional array to store floats */

  Matrix(); // not implemented
  Matrix(const Matrix &copy); // not implemented

public:
  Matrix(MemPool &pool, size_t rows, size_t cols) :
    m_rows(rows), m_cols(cols) {
    m_array = pool.Allocate<T>(rows * cols);
  }

  //~Matrix(); // not implemented

  // set upper triangle
  void InitTriangle(const T &val) {
    assert(m_rows == m_cols);
    for (size_t row = 0; row < m_rows; row++) {
      for (size_t col = row; col < m_cols; col++) {
        SetValue(row, col, val);
      }
    }
  }

  // everything
  void Init(const T &val) {
    for (size_t row = 0; row < m_rows; row++) {
      for (size_t col = 0; col < m_cols; col++) {
        SetValue(row, col, val);
      }
    }
  }

  /** Returns length of the square: typically the sentence length */
  inline size_t GetSize() const {
    assert(m_rows == m_cols);
    return m_rows;
  }

  inline size_t GetRows() const {
    return m_rows;
  }

  inline size_t GetCols() const {
    return m_cols;
  }

  /** Get a future cost score for a span */
  inline const T &GetValue(size_t row, size_t col) const {
    return m_array[row * m_cols + col];
  }

  inline T &GetValue(size_t row, size_t col) {
    return m_array[row * m_cols + col];
  }

  /** Set a future cost score for a span */
  inline void SetValue(size_t row, size_t col, const T &value) {
    m_array[row * m_cols + col] = value;
  }
};

}


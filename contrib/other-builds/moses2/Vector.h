/*
 * Vector.h
 *
 *  Created on: 7 Dec 2015
 *      Author: hieu
 */

#pragma once
#include <cassert>
#include "MemPool.h"

namespace Moses2
{

template <typename T>
class Vector {
public:
  typedef T* iterator;
  typedef const T* const_iterator;

  T *begin()
  { return m_arr; }
  const T *begin() const
  { return m_arr; }

  T *end()
  { return m_arr + m_size; }
  const T *end() const
  { return m_arr + m_size; }

  Vector(MemPool &pool, size_t size)
  :m_size(size)
  ,m_maxSize(size)
  {
	  m_arr = pool.Allocate<T>(size);
  }

  Vector(MemPool &pool, const std::vector<T> &vec)
  :m_size(vec.size())
  ,m_maxSize(vec.size())
  {
	  m_arr = pool.Allocate<T>(m_size);
	  for (size_t i = 0; i < m_size; ++i) {
		  m_arr[i] = vec[i];
	  }
  }

  virtual ~Vector()
  {

  }

  T& operator[](size_t ind)
  { return m_arr[ind]; }
  const T& operator[](size_t ind) const
  { return m_arr[ind]; }

  size_t size() const
  { return m_size; }

  void resize(size_t newSize)
  {
	  assert(newSize <= m_maxSize);
	  m_size = newSize;
  }

protected:
  size_t m_size, m_maxSize;
  T *m_arr;
};

}


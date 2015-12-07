/*
 * Vector.h
 *
 *  Created on: 7 Dec 2015
 *      Author: hieu
 */

#pragma once
#include "MemPool.h"

template <typename T>
class Vector {
public:
  Vector(MemPool &pool, size_t size)
  :m_size(size)
  {
	  m_arr = pool.Allocate<T>(size);
  }

  virtual ~Vector()
  {

  }

protected:
  size_t m_size;
  T *m_arr;
};


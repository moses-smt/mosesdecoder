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
class Vector : public std::vector<T>
{
  typedef std::vector<T> Parent;

public:
  Vector(MemPool &pool, size_t size = 0, const T &val = T())
  :Parent(size, val)
  {
  }

protected:
};

}


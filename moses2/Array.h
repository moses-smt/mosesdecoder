#pragma once
#include <cassert>
#include <boost/functional/hash.hpp>
#include "MemPool.h"

namespace Moses2
{

template<typename T>
class Array
{
public:
  typedef T* iterator;
  typedef const T* const_iterator;
  //! iterators
  const_iterator begin() const {
    return m_arr;
  }
  const_iterator end() const {
    return m_arr + m_size;
  }

  iterator begin() {
    return m_arr;
  }
  iterator end() {
    return m_arr + m_size;
  }

  Array(MemPool &pool, size_t size = 0, const T &val = T()) {
    m_size = size;
    m_maxSize = size;
    m_arr = pool.Allocate<T>(size);
    for (size_t i = 0; i < size; ++i) {
      m_arr[i] = val;
    }
  }

  size_t size() const {
    return m_size;
  }

  const T& operator[](size_t ind) const {
    return m_arr[ind];
  }

  T& operator[](size_t ind) {
    return m_arr[ind];
  }

  T *GetArray() {
    return m_arr;
  }

  size_t hash() const {
    size_t seed = 0;
    for (size_t i = 0; i < m_size; ++i) {
      boost::hash_combine(seed, m_arr[i]);
    }
    return seed;
  }

  int Compare(const Array &compare) const {

    int cmp = memcmp(m_arr, compare.m_arr, sizeof(T) * m_size);
    return cmp;
  }

  bool operator==(const Array &compare) const {
    int cmp = Compare(compare);
    return cmp == 0;
  }

  void resize(size_t newSize) {
    assert(m_size < m_maxSize);
    m_size = newSize;
  }
protected:
  size_t m_size, m_maxSize;
  T *m_arr;
};

}

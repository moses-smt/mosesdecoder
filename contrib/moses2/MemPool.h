/*
 * MemPool.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <algorithm>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <limits>
#include <iostream>

namespace Moses2
{

class MemPool
{
  struct Page
  {
    uint8_t *mem;
    uint8_t *end;
    size_t size;

    Page()
    {
    }
    Page(std::size_t size);
    ~Page();
  };

public:
  MemPool(std::size_t initSize = 10000);

  ~MemPool();

  uint8_t *Allocate(std::size_t size)
  {
    size = (size + 3) & 0xfffffffc;

    uint8_t *ret = current_;
    current_ += size;

    Page &page = *m_pages[m_currPage];
    if (current_ <= page.end) {
      // return what we got
    }
    else {
      ret = More(size);
    }
    return ret;

  }

  template<typename T>
  T *Allocate()
  {
    uint8_t *ret = Allocate(sizeof(T));
    return (T*) ret;
  }

  template<typename T>
  T *Allocate(size_t num)
  {
    uint8_t *ret = Allocate(sizeof(T) * num);
    return (T*) ret;
  }

  // re-use pool
  void Reset();

private:
  uint8_t *More(std::size_t size);

  std::vector<Page*> m_pages;

  size_t m_currSize;
  size_t m_currPage;
  uint8_t *current_;

  // no copying
  MemPool(const MemPool &);
  MemPool &operator=(const MemPool &);
};

////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
class ObjectPoolContiguous
{

public:
  ObjectPoolContiguous(std::size_t initSize = 100000) :
      m_size(0), m_actualSize(initSize)
  {
    m_vec = (T*) malloc(sizeof(T) * initSize);
  }

  ~ObjectPoolContiguous()
  {
    free(m_vec);
  }

  void Add(T &obj)
  {
    if (m_size >= m_actualSize) {
      //std::cerr << std::endl << "MORE " << m_size << std::endl;
      m_actualSize *= 2;
      m_vec = (T*) realloc(m_vec, sizeof(T) * m_actualSize);

    }
    m_vec[m_size] = obj;
    ++m_size;
  }

  bool IsEmpty() const
  {
    return m_size == 0;
  }

  void Reset()
  {
    m_size = 0;
  }

  // vector op
  size_t GetSize() const
  {
    return m_size;
  }

  const T& operator[](size_t ind) const
  {
    return m_vec[ind];
  }

  // stack op
  const T &Get() const
  {
    return m_vec[m_size - 1];
  }

  void Pop()
  {
    --m_size;
  }

  T *GetData()
  {
    return m_vec;
  }

  template<typename ORDERER>
  void Sort(const ORDERER &orderer)
  {
    std::sort(m_vec, m_vec + m_size, orderer);
  }

private:
  T *m_vec;
  size_t m_size, m_actualSize;

  // no copying
  ObjectPoolContiguous(const ObjectPoolContiguous &);
  ObjectPoolContiguous &operator=(const ObjectPoolContiguous &);
};

//////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
class MemPoolAllocator
{
public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  template<class U>
  struct rebind
  {
    typedef MemPoolAllocator<U> other;
  };

  MemPoolAllocator(Moses2::MemPool &pool) :
      m_pool(pool)
  {
  }
  MemPoolAllocator(const MemPoolAllocator &other) :
      m_pool(other.m_pool)
  {
  }

  template<class U>
  MemPoolAllocator(const MemPoolAllocator<U>& other) :
      m_pool(other.m_pool)
  {
  }

  size_type max_size() const
  {
    return std::numeric_limits<size_type>::max();
  }

  void deallocate(pointer p, size_type n)
  {
    //std::cerr << "deallocate " << p << " " << n << std::endl;
  }

  pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0)
  {
    //std::cerr << "allocate " << n << " " << hint << std::endl;
    pointer ret = m_pool.Allocate<T>(n);
    return ret;
  }

  void construct(pointer p, const_reference val)
  {
    //std::cerr << "construct " << p << " " << n << std::endl;
    new ((void *) p) T(val);
  }

  void destroy(pointer p)
  {
    //std::cerr << "destroy " << p << " " << n << std::endl;
  }

  // return address of values
  pointer address (reference value) const {
    return &value;
  }
  const_pointer address (const_reference value) const {
    return &value;
  }

  MemPool &m_pool;
protected:
};

}


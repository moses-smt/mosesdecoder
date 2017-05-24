#pragma once
#include "MemPool.h"

namespace Moses2
{

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
  struct rebind {
    typedef MemPoolAllocator<U> other;
  };

  MemPoolAllocator(Moses2::MemPool &pool) :
    m_pool(pool) {
  }
  MemPoolAllocator(const MemPoolAllocator &other) :
    m_pool(other.m_pool) {
  }

  template<class U>
  MemPoolAllocator(const MemPoolAllocator<U>& other) :
    m_pool(other.m_pool) {
  }

  size_type max_size() const {
    return std::numeric_limits<size_type>::max();
  }

  void deallocate(pointer p, size_type n) {
    //std::cerr << "deallocate " << p << " " << n << std::endl;
  }

  pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0) {
    //std::cerr << "allocate " << n << " " << hint << std::endl;
    pointer ret = m_pool.Allocate<T>(n);
    return ret;
  }

  void construct(pointer p, const_reference val) {
    //std::cerr << "construct " << p << " " << n << std::endl;
    new ((void *) p) T(val);
  }

  void destroy(pointer p) {
    //std::cerr << "destroy " << p << " " << n << std::endl;
  }

  // return address of values
  pointer address (reference value) const {
    return &value;
  }
  const_pointer address (const_reference value) const {
    return &value;
  }

  bool operator==(const MemPoolAllocator<T> &allocator) const {
    return true;
  }

  bool operator!=(const MemPoolAllocator<T> &allocator) const {
    return false;
  }

  MemPoolAllocator<T>& operator=(const MemPoolAllocator<T>& allocator) {
    return *this;
  }

  MemPool &m_pool;
protected:
};

}



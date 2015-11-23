/*
 * MemPool.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef MEMPOOL_H_
#define MEMPOOL_H_

#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <limits>

class MemPool {
	struct Page {
		uint8_t *mem;
		uint8_t *end;
		size_t size;

		Page(std::size_t size);
		~Page();
	};

  public:
	MemPool(std::size_t initSize = 10000);

    ~MemPool();

    void *Allocate(std::size_t size) {
      void *ret = current_;
      current_ += size;

      Page &page = m_pages[m_currPage];
      if (current_ < page.end) {
    	  // return what we got
      } else {
        ret = More(size);
      }
      return ret;

    }

    template<typename T>
    T *Allocate() {
    	void *ret = Allocate(sizeof(T));
    	return (T*) ret;
    }

    template<typename T>
    T *Allocate(size_t num) {
    	void *ret = Allocate(sizeof(T) * num);
    	return (T*) ret;
    }

    void Reset()
    {
    	m_currPage = 0;
    	current_ = m_pages[0].mem;
    }
  private:
    void *More(std::size_t size);

    std::vector<Page> m_pages;

    size_t m_currSize;
    size_t m_currPage;
    uint8_t *current_;

    // no copying
    MemPool(const MemPool &);
    MemPool &operator=(const MemPool &);
};

////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
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

	template< class U >
	struct rebind { typedef MemPoolAllocator<U> other; };

	MemPoolAllocator() {}
	MemPoolAllocator(const MemPoolAllocator &other) {}

	template< class U >
	MemPoolAllocator( const MemPoolAllocator<U>& other ) {}

	size_type max_size() const
	{ return std::numeric_limits<size_type>::max(); }

	void deallocate( pointer p, size_type n )
	{
		//std::cerr << "deallocate " << p << " " << n << std::endl;
	}

	pointer allocate( size_type n, std::allocator<void>::const_pointer hint = 0 )
	{
		//std::cerr << "allocate " << n << " " << hint << std::endl;
		pointer ret = m_pool.Allocate<T>(n);
		return ret;
	}


protected:
	MemPool m_pool;
};

////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class ObjectPoolContiguous {

  public:
	ObjectPoolContiguous(std::size_t initSize = 10000)
	:m_maxSize(initSize)
	{
		m_ind = 0;
		current_ = (T*) malloc(sizeof(T) * initSize);
	}

    ~ObjectPoolContiguous()
    {
    	free(current_);
    }

    void push(T &obj) {
    	T &mem = Allocate();
    	mem = obj;
    	++m_ind;
    }

    void clear()
    {
    	m_ind = 0;
    }

    size_t size() const
    { return m_ind; }

    T &get(size_t ind) const {
    	return current_[ind];
    }
  private:
    size_t m_maxSize;
    size_t m_ind;
    T *current_;

    T &Allocate() {
      if (m_ind >= m_maxSize) {
    	  m_maxSize <<= 1;
    	  current_ = (T*) realloc(current_, m_maxSize);
      }
      ++m_ind;

      return current_[m_ind];
    }

    // no copying
    ObjectPoolContiguous(const ObjectPoolContiguous &);
    ObjectPoolContiguous &operator=(const ObjectPoolContiguous &);
};

#endif /* MEMPOOL_H_ */

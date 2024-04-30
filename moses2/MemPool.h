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
  struct Page {
    uint8_t *mem;
    uint8_t *end;
    size_t size;

    Page() = delete;
    Page(std::size_t size);
    ~Page();
  };

public:
  MemPool(std::size_t initSize = 10240);

  ~MemPool();

  uint8_t* Allocate(std::size_t size);

  template<typename T>
  T *Allocate() {
    uint8_t *ret = Allocate(sizeof(T));
    return (T*) ret;
  }

  template<typename T>
  T *Allocate(size_t num) {
    size_t size = sizeof(T);
    size_t m = size % 16;
    size += m;

    uint8_t *ret = Allocate(size * num);
    return (T*) ret;
  }

  // re-use pool
  void Reset();

  size_t Size();

private:
  uint8_t *More(std::size_t size);

  std::vector<Page*> m_pages;

  size_t m_currSize;
  size_t m_currPage;
  uint8_t *current_;

  // no copying
  MemPool(const MemPool &) = delete;
  MemPool &operator=(const MemPool &) = delete;
};


}


/*
 * MemPool.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "MemPool.h"
#include "util/scoped.hh"
#include "legacy/Util2.h"

using namespace std;

namespace Moses2
{

MemPool::Page::Page(std::size_t vSize) :
  size(vSize)
{
  mem = (uint8_t*) util::MallocOrThrow(size);
  end = mem + size;
}

MemPool::Page::~Page()
{
  free(mem);
}
////////////////////////////////////////////////////
MemPool::MemPool(size_t initSize) :
  m_currSize(initSize), m_currPage(0)
{
  Page *page = new Page(m_currSize);
  m_pages.push_back(page);

  current_ = page->mem;
  //cerr << "new memory pool";
}

MemPool::~MemPool()
{
  //cerr << "delete memory pool " << m_currSize << endl;
  RemoveAllInColl(m_pages);
}

uint8_t* MemPool::Allocate(std::size_t size) {
  if (size == 0) {
    return nullptr;
  }
  //size = (size + 3) & 0xfffffffc;
  //size = (size + 7) & 0xfffffff8;
  size = (size + 15) & 0xfffffff0;
  //size = (size + 31) & 0xffffffe0;

  uint8_t* ret = current_;
  current_ += size;

  assert(m_currPage < m_pages.size());
  Page& page = *m_pages[m_currPage];
  if (current_ <= page.end) {
    // return what we got
  }
  else {
    ret = More(size);
  }
  return ret;

}

uint8_t *MemPool::More(std::size_t size)
{
  ++m_currPage;
  if (m_currPage >= m_pages.size()) {
    // add new page
    m_currSize <<= 1;
    std::size_t amount = std::max(m_currSize, size);

    Page *page = new Page(amount);
    //cerr << "NEW PAGE " << amount << endl;
    m_pages.push_back(page);

    uint8_t *ret = page->mem;
    current_ = ret + size;
    return ret;
  } else {
    // use existing page
    Page &page = *m_pages[m_currPage];
    if (size <= page.size) {
      uint8_t *ret = page.mem;
      current_ = ret + size;
      return ret;
    } else {
      // recursive call More()
      return More(size);
    }
  }
}

void MemPool::Reset()
{
  if (m_pages.size() > 1) {
    size_t total = 0;
    for (size_t i = 0; i < m_pages.size(); ++i) {
      total += m_pages[i]->size;
    }
    RemoveAllInColl(m_pages);
    Page* page = new Page(total);
    m_pages.push_back(page);
  }

  m_currPage = 0;
  current_ = m_pages[0]->mem;
}

size_t MemPool::Size()
{
  size_t ret = 0;
  for (const Page *page: m_pages) {
    ret += page->size;
  }
  return ret;
}

}


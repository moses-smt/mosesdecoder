/*
 * MemPool.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include "MemPool.h"
#include "util/scoped.hh"

MemPool::Page::Page(std::size_t vSize)
:size(vSize)
{
  mem = (uint8_t*) util::MallocOrThrow(size);
  end = mem + size;
}

MemPool::Page::~Page()
{

}
////////////////////////////////////////////////////
MemPool::MemPool(size_t initSize)
:m_currSize(initSize)
,m_currPage(0)
{
	m_pages.push_back(Page(m_currSize));
}

MemPool::~MemPool() {

}


void *MemPool::More(std::size_t size)
{
	++m_currPage;
	if (m_currPage >= m_pages.size()) {
		// add new page
		m_currSize <<= 1;
		std::size_t amount = std::max(m_currSize, size);
		m_pages.push_back(Page(amount));

		Page &page = m_pages.back();
		uint8_t *ret = page.mem;
		current_ = ret + size;
		return ret;
	}
	else {
		// use existing page
		Page &page = m_pages[m_currPage];
		if (size <= page.size) {
			uint8_t *ret = page.mem;
			current_ = ret + size;
			return ret;
		}
		else {
			// recursive call More()
			return More(size);
		}
	}
}

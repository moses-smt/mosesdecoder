#ifndef __n_best_list_h
#define __n_best_list_h
#include <algorithm>
#include "moses/TranslationModel/UG/generic/sorting/VectorIndexSorter.h"

// NBest List; (c) 2007-2012 Ulrich Germann
//
// The 'trick' used in this implementation is to maintain a heap of size <= N
// such that the lowest-scoring item is on top of the heap. For each incoming
// item we can then determine easily if it is in the top N.

namespace Moses
{
using namespace std;

template<typename THINGY, typename CMP>
class
  NBestList
{
  vector<uint32_t> m_heap;
  vector<THINGY>   m_list;
  VectorIndexSorter<THINGY, CMP, uint32_t> m_better;
  mutable vector<uint32_t> m_order;
  mutable bool m_changed;
public:
  NBestList(size_t const max_size, CMP const& cmp);
  NBestList(size_t const max_size);
  bool add(THINGY const& item);
  THINGY const& operator[](int i) const;
  size_t size() const {
    return m_heap.size();
  }
};

template<typename THINGY, typename CMP>
NBestList<THINGY,CMP>::
NBestList(size_t const max_size, CMP const& cmp)
  : m_better(m_list, cmp), m_changed(false)
{
  m_heap.reserve(max_size);
}

template<typename THINGY, typename CMP>
NBestList<THINGY,CMP>::
NBestList(size_t const max_size)
  : m_better(m_heap), m_changed(false)
{
  m_heap.reserve(max_size);
}

template<typename THINGY, typename CMP>
bool
NBestList<THINGY,CMP>::
add(THINGY const& item)
{
  if (m_heap.size() == m_heap.capacity()) {
    if (m_better.Compare(item, m_list[m_heap.at(0)])) {
      pop_heap(m_heap.begin(),m_heap.end(),m_better);
      m_list[m_heap.back()] = item;
    } else return false;
  } else {
    m_list.push_back(item);
    m_heap.push_back(m_heap.size());
  }
  push_heap(m_heap.begin(),m_heap.end(),m_better);
  return m_changed = true;
}

template<typename THINGY, typename CMP>
THINGY const&
NBestList<THINGY,CMP>::
operator[](int i) const
{
  if (m_changed) {
    m_order.assign(m_heap.begin(),m_heap.end());
    for (size_t k = m_heap.size(); k != 0; --k)
      pop_heap(m_order.begin(), m_order.begin()+k);
    m_changed = false;
  }
  if (i < 0) i += m_order.size();
  return m_list[m_order.at(i)];
}

}
#endif

//-*- c++ -*-
#pragma once
#include <vector>
#include <map>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <sys/time.h>


#ifndef SPTR
#define SPTR boost::shared_ptr
#endif

namespace lru_cache
{

  template<typename KEY, typename VAL>
  class LRU_Cache
  {
  public:
    typedef boost::unordered_map<KEY,uint32_t> map_t;
  private:
    struct Record
    {
      uint32_t prev,next;
      KEY            key;
      // timeval      tstamp; // time stamp
      typename boost::shared_ptr<VAL> ptr; // cached shared ptr
    };

    mutable boost::shared_mutex m_lock;
    uint32_t m_qfront, m_qback;
    std::vector<Record> m_recs;
    map_t m_idx;

    void
    update_queue(KEY const& key, uint32_t const p)
    {
      // CALLER MUST LOCK!
      // "remove" item in slot p from it's current position of the
      // queue (which is different from the slot position) and move it
      // to the end
      Record& r = m_recs[p];
      if (m_recs.size() == 1)
	r.next = r.prev = m_qback = m_qfront = 0;

      if (r.key != key || p == m_qback) return;

      if (m_qfront == p)
	m_qfront = m_recs[r.next].prev = r.next;
      else
	{
	  m_recs[r.prev].next = r.next;
	  m_recs[r.next].prev = r.prev;
	}
      r.prev = m_qback;
      m_recs[r.prev].next = m_qback = r.next = p;
    }

  public:
    LRU_Cache(size_t capacity=1) : m_qfront(0), m_qback(0) { reserve(capacity); }
    size_t capacity() const { return m_recs.capacity(); }
    size_t size() const { return m_idx.size(); }
    void reserve(size_t s) { m_recs.reserve(s); }

    SPTR<VAL>
    get(KEY const& key)
    {
      uint32_t p;
      { // brackets needed for lock scoping
	boost::shared_lock<boost::shared_mutex> rlock(m_lock);
	typename map_t::const_iterator i = m_idx.find(key);
	if (i == m_idx.end()) return SPTR<VAL>();
	p = i->second;
      }
      boost::lock_guard<boost::shared_mutex> guard(m_lock);
      update_queue(key,p);
      return m_recs[p].ptr;
    }

    void
    set(KEY const& key, SPTR<VAL> const& ptr)
    {
      boost::lock_guard<boost::shared_mutex> lock(m_lock);
      std::pair<typename map_t::iterator,bool> foo;
      foo = m_idx.insert(make_pair(key,m_recs.size()));
      uint32_t p = foo.first->second;
      if (foo.second) // was not in the cache
	{
	  if (m_recs.size() < m_recs.capacity())
	    m_recs.push_back(Record());
	  else
	    {
	      foo.first->second = p = m_qfront;
	      m_idx.erase(m_recs[p].key);
	    }
	  m_recs[p].key = key;
	}
      update_queue(key,p);
      m_recs[p].ptr = ptr;
    }
  };
}

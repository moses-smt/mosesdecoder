// -*- c++ -*-
// A class to store "local" information (such as task-specific caches).
// The idea is for each translation task to have a scope, which stores
// shared pointers to task-specific objects such as caches and priors.
// Since these objects are referenced via shared pointers, sopes can
// share information.
#pragma once

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/foreach.hpp>
#endif

#include <map>
#include <boost/shared_ptr.hpp>
// #include "thread_safe_container.h"

namespace Moses
{
class ContextScope
{
protected:
  typedef std::map<void const*, boost::shared_ptr<void> > scratchpad_t;
  typedef scratchpad_t::iterator iter_t;
  typedef scratchpad_t::value_type entry_t;
  typedef scratchpad_t::const_iterator const_iter_t;
  scratchpad_t m_scratchpad;
#ifdef WITH_THREADS
  mutable boost::shared_mutex m_lock;
#endif
public:
  // class write_access
  // {
  //   boost::unique_lock<boost::shared_mutex> m_lock;
  // public:

  //   write_access(boost::shared_mutex& lock)
  // 	: m_lock(lock)
  //   { }

  //   write_access(write_access& other)
  //   {
  // 	swap(m_lock, other.m_lock);
  //   }
  // };

  // write_access lock() const
  // {
  //   return write_access(m_lock);
  // }

  template<typename T>
  boost::shared_ptr<void> const&
  set(void const* const key, boost::shared_ptr<T> const& val) {
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
#endif
    return (m_scratchpad[key] = val);
  }

  template<typename T>
  boost::shared_ptr<T> const
  get(void const* key, bool CreateNewIfNecessary=false) {
#ifdef WITH_THREADS
    using boost::shared_mutex;
    using boost::upgrade_lock;
    // T const* key = reinterpret_cast<T const*>(xkey);
    upgrade_lock<shared_mutex> lock(m_lock);
#endif
    iter_t m = m_scratchpad.find(key);
    boost::shared_ptr< T > ret;
    if (m != m_scratchpad.end()) {
      if (m->second == NULL && CreateNewIfNecessary) {
#ifdef WITH_THREADS
        boost::upgrade_to_unique_lock<shared_mutex> xlock(lock);
#endif
        m->second.reset(new T);
      }
      ret = boost::static_pointer_cast< T >(m->second);
      return ret;
    }
    if (!CreateNewIfNecessary) return ret;
#ifdef WITH_THREADS
    boost::upgrade_to_unique_lock<shared_mutex> xlock(lock);
#endif
    ret.reset(new T);
    m_scratchpad[key] = ret;
    return ret;
  }

  ContextScope() { }

  ContextScope(ContextScope const& other) {
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock1(this->m_lock);
    boost::unique_lock<boost::shared_mutex> lock2(other.m_lock);
#endif
    m_scratchpad = other.m_scratchpad;
  }

};

};

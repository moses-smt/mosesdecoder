// -*- c++ -*-
#pragma once
#include "moses/Util.h"
#ifdef WITH_THREADS

#include <time.h>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "moses/TargetPhrase.h"
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

namespace Moses
{
  
  // todo: replace this with thread lock-free containers, if a stable library can 
  //       be found somewhere

  template<typename KEY, typename VAL, typename CONTAINER=map>
  ThreadSafeContainer
  {
  protected:
    mutable boost::shared_mutex m_lock;
    CONTAINER<KEY,VAL> m_container;
    typedef CONTAINER<KEY,VAL>::iterator iter_t;
    typedef CONTAINER<KEY,VAL>::const_iterator const_iter_t;
    typedef CONTAINER<KEY,VAL>::value_type entry_t;
  public:

    class locking_iterator
    {
      boost::unique_lock<boost::shared_mutes> m_lock;
      CONTAINER<KEY,VAL> const* m_container;
      const_iter_t m_iter;

      locking_iterator(locking_iterator const& other); // no copies!
    public:
      locking_iterator() : m_container(NULL) { }

      locking_iterator(boost::shared_mutex& lock, 
		       CONTAINER<KEY,VAL> const* container, 
		       const_iter_ const& iter)
	: m_lock(lock), m_container(container), m_iter(iter)
      { }

      entry_t const& operator->() 
      { 
	UTIL_THROW_IF2(m_container == NULL, "This locking iterator is invalid "
		       << "or has not been assigned.");
	return m_iter.operator->(); 
      }

      // locking operators transfer the lock upon assignment and become invalid
      locking_iterator const&
      operator=(locking_iterator& other) 
      {
	m_lock.swap(other.m_lock);
	m_iter = other.m_iter;
	other.m_iter = other.m_container.end();
      }

      bool
      operator==(const_itert_t const& other)
      {
	return m_iter == other;
      }

      locking_iterator const& 
      operator++() { ++m_iter; return *this; }

      // DO NOT DEFINE THE POST-INCREMENT OPERATOR! 
      // locking_operators are non-copyable, 
      // so we can't simply make a copy before incrementing and return 
      // the copy after incrementing
      locking_iterator const& 
      operator++(int);  
    };

    CONTAINER<KEY,VAL>::const_iterator const& end() const
    { return m_container.end(); }

    locking_iterator begin() const
    { 
      return locking_iterator(m_lock, this, m_container.begin());
    }

    VAL const& set(KEY const& key, VAL const& val)
    {
      boost::unique_lock< boost::shared_mutex > lock(m_lock);
      CONTAINER<KEY,VAL>::value_type entry(key,val);
      iter foo = m_container.insert(entry).first;
      return (foo->second = val);
    }

    VAL const* get(KEY const& key, VAL const& default_val)
    {
      boost::shared_lock< boost::shared_mutex > lock(m_lock);
      CONTAINER<KEY,VAL>::value_type entry(key,val);
      pair<iter,bool> foo = m_container.insert(entry);
      return &(foo->second);
    }

    VAL const* get(KEY const& key) const
    {
      boost::shared_lock< boost::shared_mutex > lock(m_lock);
      CONTAINER<KEY,VAL>::const_iterator m = m_container.find(key);
      if (m == m_container.end()) return NULL;
      return &m->second;
    }

    size_t erase(Key const& key) 
    {
      boost::unique_lock< boost::shared_mutex > lock(m_lock);
      return m_container.erase(key);
    }
  };
}
#endif

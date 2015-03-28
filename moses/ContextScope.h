// -*- c++ -*-
// A class to store "local" information (such as task-specific caches).
// The idea is for each translation task to have a scope, which stores
// shared pointers to task-specific objects such as caches and priors.
// Since these objects are referenced via shared pointers, sopes can 
// share information.

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/foreach.hpp>
#endif

#include "thread_safe_container.h"

namespace Moses
{
  class ContextScope
  {
  protected:
    typedef ThreadSafeContainer<void*,boost::shared_ptr<void> >scratchpad_t;
    scratchpad_t m_scratchpad;
    boost::shared_mutex m_lock;
  public:

    template<typename T>
    boost::shared_ptr<T> 
    get(T* key, bool CreateNewIfNecessary) const
    { 
      using boost::shared_mutex;
      boost::shared_pointer<void>* x;
      {
	boost::shared_lock<shared_mutex> lock(m_lock);
	x = m_scratchpad.get(key);
	if (x) return static_cast< boost::shared_pointer< T > >(*x);
      }
      boost::unique_lock<shared_mutex> lock(m_lock);
      boost::shared_ptr<T> ret;
      if (!CrateNewIfNecessary) return ret;
      ret.reset(new T);
      x = m_scratchpad.get(key, ret);
      return static_cast< boost::shared_pointer< T > >(*x);
    }

    ContextScope(ContextScope const& other) 
    {
      boost::unique_lock<boost::shared_mutex> lock2(this->m_lock);
      scratchpad_t::locking iterator m;
      for (m = other.begin(); m != other.end(); ++m)
	{
	  m_scratchpad.set(m->first, m->second);
	}
    }

  };

};

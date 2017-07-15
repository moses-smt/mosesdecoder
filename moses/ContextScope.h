// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
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

// for some reason, the xmlrpc_c headers must be included AFTER the
// boost thread-related ones ...
#include "xmlrpc-c.h"

#include <map>
#include <boost/shared_ptr.hpp>
#include "TypeDef.h"
#include "Util.h"

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
  SPTR<std::map<std::string,float> const> m_context_weights;
public:
  typedef boost::shared_ptr<ContextScope> ptr;
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

  SPTR<std::map<std::string,float> const>
  GetContextWeights() {
    return m_context_weights;
  }

  bool
  SetContextWeights(std::string const& spec) {
    if (m_context_weights) return false;
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
    SPTR<std::map<std::string,float> > M(new std::map<std::string, float>);

    // TO DO; This needs to be done with StringPiece.find, not Tokenize
    // PRIORITY: low
    std::vector<std::string> tokens = Tokenize(spec,":");
    for (std::vector<std::string>::iterator it = tokens.begin();
         it != tokens.end(); it++) {
      std::vector<std::string> key_and_value = Tokenize(*it, ",");
      (*M)[key_and_value[0]] = atof(key_and_value[1].c_str());
    }
    m_context_weights = M;
    return true;
  }

  bool
  SetContextWeights(SPTR<std::map<std::string,float> const> const& w) {
    if (m_context_weights) return false;
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
#endif
    m_context_weights = w;
    return true;
  }

};

};

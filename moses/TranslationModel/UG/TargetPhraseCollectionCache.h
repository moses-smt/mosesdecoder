// -*- c++ -*-
#pragma once
#include <time.h>
#include "moses/TargetPhraseCollection.h"
#include <boost/atomic.hpp>
#include "mm/ug_typedefs.h"
namespace Moses
{

  class TPCollWrapper;

  class TPCollCache
  {
  public:
    // typedef boost::unordered_map<uint64_t, SPTR<TPCollWrapper> > cache_t;
    typedef std::map<uint64_t, SPTR<TPCollWrapper> > cache_t;
  private:
    uint32_t m_capacity; // capacity of cache
    cache_t     m_cache; // maps from ids to items
    cache_t::iterator m_qfirst, m_qlast;
    mutable boost::shared_mutex  m_lock; 
  public:
    TPCollCache(size_t capacity=10000);

    SPTR<TPCollWrapper>
    get(uint64_t key, size_t revision);

  };

  // wrapper around TargetPhraseCollection with reference counting
  // and additional members for caching purposes
  class TPCollWrapper
    : public TargetPhraseCollection
  {
    friend class TPCollCache;
    friend class Mmsapt;
  public:
    TPCollCache::cache_t::iterator prev, next;
  public:
    mutable boost::shared_mutex lock; 
    size_t   const revision; // rev. No. of the underlying corpus
    uint64_t const      key; // phrase key
    TPCollWrapper(uint64_t const key, size_t const rev);
    ~TPCollWrapper();
  };

}

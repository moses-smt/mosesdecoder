// -*- c++ -*-
#pragma once
#include <time.h>
#include "moses/TargetPhraseCollection.h"
#include <boost/atomic.hpp>

namespace Moses
{

  class TPCollCache;

  class TPCollWrapper
  // wrapper around TargetPhraseCollection with reference counting
  // and additional members for caching purposes
    : public TargetPhraseCollection
  {
    friend class TPCollCache;
    friend class Mmsapt;
    mutable boost::atomic<uint32_t> refCount; // reference count
  public:
    TPCollWrapper*  prev; // ... in queue of TPCollWrappers used recently
    TPCollWrapper*  next; // ... in queue of TPCollWrappers used recently
  public:
    mutable boost::shared_mutex lock; 
    size_t   const revision; // rev. No. of the underlying corpus
    uint64_t const      key; // phrase key
#if defined(timespec) // timespec is better, but not available everywhere
    timespec         tstamp; // last use
#else
    timeval          tstamp; // last use
#endif
    TPCollWrapper(uint64_t const key, size_t const rev);
    ~TPCollWrapper();
  };

  class TPCollCache
  {
    typedef boost::unordered_map<uint64_t, TPCollWrapper*> cache_t;
    typedef std::vector<TPCollWrapper*> history_t;
    cache_t   m_cache;   // maps from phrase ids to target phrase collections
    // mutable history_t m_history; // heap of live items, least recently used one on top

    mutable boost::shared_mutex m_lock;   // locks m_cache

    TPCollWrapper* m_doomed_first;
    TPCollWrapper* m_doomed_last;
    uint32_t m_doomed_count; // counter of doomed TPCs
    uint32_t m_capacity;     // capacity of cache
    void add_to_queue(TPCollWrapper* x);
    void remove_from_queue(TPCollWrapper* x);
  public:
    TPCollCache(size_t capacity=10000);

    TPCollWrapper*
    get(uint64_t key, size_t revision);

    void
    release(TPCollWrapper const* tpc);
  };


}

// -*- c++ -*-
#pragma once
#include <time.h>
#include "moses/TargetPhraseCollection.h"

namespace Moses
{
  class TPCollWrapper
  // wrapper around TargetPhraseCollection that includes reference counts
  // and a time stamp for least-recently-used caching of TargetPhraseCollection-s
    : public TargetPhraseCollection
  {
  public:
    size_t   const revision;
    // revison; gets changed when the underlying corpus in Mmsapt is updated

    uint64_t const      key; // phrase key
    uint32_t       refCount; // reference count
#if defined(timespec) // timespec is better, but not available everywhere
    timespec         tstamp; // last use
#else
    timeval          tstamp; // last use
#endif
    int                 idx; // position in the history heap
    TPCollWrapper(size_t r, uint64_t const k);
    ~TPCollWrapper();
  };

  class TPCollCache
  {
    typedef boost::unordered_map<uint64_t, TPCollWrapper*> cache_t;
    typedef std::vector<TPCollWrapper*> history_t;
    cache_t   m_cache;   // maps from phrase ids to target phrase collections
    mutable history_t m_history; // heap of live items, least recently used one on top

    mutable boost::shared_mutex m_cache_lock;   // locks m_cache
    mutable boost::shared_mutex m_history_lock; // locks m_history

#if 0
    // mutable size_t m_tpc_ctr;
    // counter of all live item, for debugging. probably obsolete; was used
    // to track memory leaks
#endif

    TPCollWrapper* encache(TPCollWrapper* const& ptr);
    // updates time stamp and position in least-recently-used heap m_history

  public:
    TPCollCache(size_t capacity=1000);

    TPCollWrapper*
    get(uint64_t key, size_t revision);

    void
    add(uint64_t key, TPCollWrapper* ptr);

    void
    release(TPCollWrapper*& tpc);
  };


}

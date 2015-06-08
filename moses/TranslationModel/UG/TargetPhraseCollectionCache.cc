#include "TargetPhraseCollectionCache.h"

namespace Moses
{
  using std::vector;

#if defined(timespec)
  bool operator<(timespec const& a, timespec const& b)
  {
    if (a.tv_sec != b.tv_sec) return a.tv_sec < b.tv_sec;
    return (a.tv_nsec < b.tv_nsec);
  }

  bool operator>=(timespec const& a, timespec const& b)
  {
    if (a.tv_sec != b.tv_sec) return a.tv_sec > b.tv_sec;
    return (a.tv_nsec >= b.tv_nsec);
  }
#endif

  bool operator<(timeval const& a, timeval const& b)
  {
    if (a.tv_sec != b.tv_sec) return a.tv_sec < b.tv_sec;
    return (a.tv_usec < b.tv_usec);
  }

  bool operator>=(timeval const& a, timeval const& b)
  {
    if (a.tv_sec != b.tv_sec) return a.tv_sec > b.tv_sec;
    return (a.tv_usec >= b.tv_usec);
  }

  void
  bubble_up(std::vector<TPCollWrapper*>& v, size_t k)
  {
    if (k >= v.size()) return;
    for (;k && (v[k]->tstamp < v[k/2]->tstamp); k /=2)
      {
  	std::swap(v[k],v[k/2]);
  	std::swap(v[k]->idx,v[k/2]->idx);
      }
  }

  void
  bubble_down(std::vector<TPCollWrapper*>& v, size_t k)
  {
    for (size_t j = 2*(k+1); j <= v.size(); j = 2*((k=j)+1))
      {
	if (j == v.size() || (v[j-1]->tstamp < v[j]->tstamp)) --j;
	if (v[j]->tstamp >= v[k]->tstamp) break;
	std::swap(v[k],v[j]);
	v[k]->idx = k;
	v[j]->idx = j;
      }
  }

  TPCollCache
  ::TPCollCache(size_t capacity)
  {
    m_history.reserve(capacity);
  }

  TPCollWrapper*
  TPCollCache
  ::encache(TPCollWrapper* const& ptr)
  {
    using namespace boost;
    // update time stamp:
#if defined(timespec)
    clock_gettime(CLOCK_MONOTONIC, &ptr->tstamp);
#else
    gettimeofday(&ptr->tstamp, NULL);
#endif
    unique_lock<shared_mutex> lock(m_history_lock);
    if (m_history.capacity() > 1)
      {
	vector<TPCollWrapper*>& v = m_history;
	if (ptr->idx >= 0) // ptr is already in history
	  {
	    assert(ptr == v[ptr->idx]);
	    size_t k = 2 * (ptr->idx + 1);
	    if (k < v.size()) bubble_up(v,k--);
	    if (k < v.size()) bubble_up(v,k);
	  }
	else if (v.size() < v.capacity())
	  {
	    size_t k = ptr->idx = v.size();
	    v.push_back(ptr);
	    bubble_up(v,k);
	  }
	else // someone else needs to go
	  {
	    v[0]->idx = -1;
	    release(v[0]);
	    v[0] = ptr;
	    bubble_down(v,0);
	  }
      }
    return ptr;
  } // TPCollCache::encache(...)

  TPCollWrapper*
  TPCollCache
  ::get(uint64_t key, size_t revision)
  {
    using namespace boost;
    cache_t::iterator m;
    {
      shared_lock<shared_mutex> lock(m_cache_lock);
      m = m_cache.find(key);
      if (m == m_cache.end() || m->second->revision != revision)
	return NULL;
      ++m->second->refCount;
    }

    encache(m->second);
    return NULL;
  } // TPCollCache::get(...)

  void
  TPCollCache
  ::add(uint64_t key, TPCollWrapper* ptr)
  {
    {
      boost::unique_lock<boost::shared_mutex> lock(m_cache_lock);
      m_cache[key] = ptr;
      ++ptr->refCount;
      // ++m_tpc_ctr;
    }
    encache(ptr);
  } // TPCollCache::add(...)

  void
  TPCollCache
  ::release(TPCollWrapper*& ptr)
  {
    if (!ptr) return;

    if (--ptr->refCount || ptr->idx >= 0) // tpc is still in use
      {
	ptr = NULL;
	return;
      }

#if 0
    timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    timespec r; clock_getres(CLOCK_MONOTONIC,&r);
    float delta = t.tv_sec - ptr->tstamp.tv_sec;
    cerr << "deleting old cache entry after " << delta << " seconds."
	 << " clock resolution is " << r.tv_sec << ":" << r.tv_nsec
	 << " at " << __FILE__ << ":" << __LINE__ << endl;
#endif

    boost::upgrade_lock<boost::shared_mutex> lock(m_cache_lock);
    cache_t::iterator m = m_cache.find(ptr->key);
    if (m != m_cache.end() && m->second == ptr)
      { // the cache could have been updated with a new pointer
	// for the same phrase already, so we need to check
	// if the pointer we cound is the one we want to get rid of,
	// hence the second check
	boost::upgrade_to_unique_lock<boost::shared_mutex> xlock(lock);
	m_cache.erase(m);
      }
    delete ptr;
    ptr = NULL;
  } // TPCollCache::release(...)

  TPCollWrapper::
  TPCollWrapper(size_t r, uint64_t k)
    : revision(r), key(k), refCount(0), idx(-1)
  { }

  TPCollWrapper::
  ~TPCollWrapper()
  {
    assert(this->refCount == 0);
  }

} // namespace

#include "TargetPhraseCollectionCache.h"

namespace Moses
{
  using std::vector;

  TPCollCache
  ::TPCollCache(size_t capacity)
  {
    m_doomed_first = m_doomed_last = NULL;
    m_doomed_count = 0;
    m_capacity = capacity;
  }

  /// remove a TPC from the "doomed" queue 
  void 
  TPCollCache
  ::remove_from_queue(TPCollWrapper* x)
  {
    // caller must lock!
    if (m_doomed_first == x) 
      m_doomed_first = x->next; 
    else (x->prev->next) = x->next;
    if (m_doomed_last  == x)
      m_doomed_last = x->prev;
    else x->next->prev = x->prev;
    x->next = x->prev = NULL;
    --m_doomed_count;
  }

  void 
  TPCollCache
  ::add_to_queue(TPCollWrapper* x)
  {
    // caller must lock!
    x->prev = m_doomed_last;
    if (!m_doomed_first) m_doomed_first = x;
    (m_doomed_last ? m_doomed_last->next : m_doomed_last) = x;
    ++m_doomed_count;
  }

  TPCollWrapper*
  TPCollCache
  ::get(uint64_t key, size_t revision)
  {
    using namespace boost;
    upgrade_lock<shared_mutex> rlock(m_lock);
    cache_t::iterator m = m_cache.find(key);
    if (m == m_cache.end()) // new
      {
	std::pair<uint64_t,TPCollWrapper*> e(key,NULL);
	upgrade_to_unique_lock<shared_mutex> wlock(rlock);
	std::pair<cache_t::iterator,bool> foo = m_cache.insert(e);
	if (foo.second) foo.first->second = new TPCollWrapper(key, revision);
	m = foo.first;
      }
    else 
      {
	if (m->second->refCount == 0)
	  {
	    upgrade_to_unique_lock<shared_mutex> wlock(rlock);
	    remove_from_queue(m->second);
	  }
	if (m->second->revision != revision) // out of date
	  {
	    upgrade_to_unique_lock<shared_mutex> wlock(rlock);
	    m->second = new TPCollWrapper(key, revision);
	  }
      }
    ++m->second->refCount;
    return m->second;
  } // TPCollCache::get(...)
  
  void
  TPCollCache
  ::release(TPCollWrapper* ptr)
  {
    if (!ptr) return;
    if (--ptr->refCount == 0)
      {
	boost::unique_lock<boost::shared_mutex> lock(m_lock);
	if (m_doomed_count == m_capacity)
	  {
	    TPCollWrapper* x = m_doomed_first;
	    remove_from_queue(x);
	    UTIL_THROW_IF2(x->refCount || x == ptr, "TPC was doomed while still in use!");
	    cache_t::iterator m = m_cache.find(ptr->key);
	    if (m != m_cache.end() && m->second == ptr)
	      { // the cache could have been updated with a new pointer
		// for the same phrase already, so we need to check
		// if the pointer we cound is the one we want to get rid of,
		// hence the second check
		// boost::upgrade_to_unique_lock<boost::shared_mutex> xlock(lock);
		m_cache.erase(m);
	      }
	    delete x;
	  }
	add_to_queue(ptr);
      }
  } // TPCollCache::release(...)
  
  TPCollWrapper::
  TPCollWrapper(size_t revision_, uint64_t key_)
    : refCount(0), prev(NULL), next(NULL)
    , revision(revision_), key(key_)
  { }

  TPCollWrapper::
  ~TPCollWrapper()
  {
    UTIL_THROW_IF2(this->refCount, "TPCollWrapper refCount > 0!");
    assert(this->refCount == 0);
  }

} // namespace

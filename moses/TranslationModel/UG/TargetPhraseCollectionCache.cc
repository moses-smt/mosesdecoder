#include "TargetPhraseCollectionCache.h"

namespace Moses
{
  using std::vector;

  TPCollCache::
  TPCollCache(size_t capacity)
  {
    m_qfirst = m_qlast = m_cache.end();
    m_capacity = capacity;
    UTIL_THROW_IF2(m_capacity <= 2, "Cache capacity must be > 1!");
  }

  SPTR<TPCollWrapper>
  TPCollCache::
  get(uint64_t key, size_t revision)
  {
    using namespace boost;
    unique_lock<shared_mutex> lock(m_lock);
    std::pair<uint64_t, SPTR<TPCollWrapper> > e(key, SPTR<TPCollWrapper>());
    std::pair<cache_t::iterator, bool> foo = m_cache.insert(e);
    SPTR<TPCollWrapper>& ret = foo.first->second;
    if (ret)
      {
	if (m_qfirst == foo.first) m_qfirst = ret->next;
	else ret->prev->second->next = ret->next;
	if (m_qlast != foo.first)
	  ret->next->second->prev = ret->prev;
      }
    if (!ret || ret->revision != revision)
      ret.reset(new TPCollWrapper(key,revision));
    ret->prev = m_qlast;
    if (m_qlast != m_cache.end()) m_qlast->second->next = foo.first;
    m_qlast = foo.first;

    while (m_cache.size() > m_capacity && m_qfirst->second.use_count() == 1)
      {
	m_qfirst = m_qfirst->second->next;
	m_cache.erase(m_qfirst->second->prev);
      }

    return ret;
  } // TPCollCache::get(...)
  
  TPCollWrapper::
  TPCollWrapper(uint64_t key_, size_t revision_)
    : revision(revision_), key(key_)
  { }

  TPCollWrapper::
  ~TPCollWrapper()
  { }

} // namespace

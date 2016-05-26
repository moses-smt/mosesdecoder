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

#if 0
    size_t ctr=0;
    std::cerr << "BEFORE" << std::endl;
    for (cache_t::iterator m = m_qfirst; m != m_cache.end(); m = m->second->next)
      {
	std::cerr << ++ctr << "/" << m_cache.size() << " " 
		  << (m->second->key == key ? "*" : " ")
		  << m->second->key << " " 
		  << m->second.use_count();
	if (m->second->prev != m_cache.end())
	  std::cerr << " => " << m->second->prev->second->key;
	std::cerr << std::endl;
      } 
    std::cerr << "\n" << std::endl;
#endif

    std::pair<uint64_t, SPTR<TPCollWrapper> > e(key, SPTR<TPCollWrapper>());
    std::pair<cache_t::iterator, bool> foo = m_cache.insert(e);
    SPTR<TPCollWrapper>& ret = foo.first->second;
    if (ret && m_cache.size() > 1 && m_qlast != foo.first)
      {
	if (m_qfirst == foo.first) m_qfirst = ret->next;
	else ret->prev->second->next = ret->next;
	if (m_qlast != foo.first)
	  ret->next->second->prev = ret->prev;
      }
    if (!ret || ret->revision != revision)
      {
	ret.reset(new TPCollWrapper(key,revision));
      }
    if (m_cache.size() == 1) 
      {
	m_qfirst = m_qlast = foo.first;
	ret->prev = m_cache.end();
      }
    else if (m_qlast != foo.first)
      {
	ret->prev = m_qlast;
	m_qlast->second->next = foo.first;
	m_qlast = foo.first;
      }
    ret->next = m_cache.end();

#if 0
    std::cerr << "AFTER" << std::endl;
    ctr=0;
    for (cache_t::iterator m = m_qfirst; m != m_cache.end(); m = m->second->next)
      {
	std::cerr << ++ctr << "/" << m_cache.size() << " " 
		  << (m->second->key == key ? "*" : " ")
		  << m->second->key << " " 
		  << m->second.use_count();
	if (m->second->prev != m_cache.end())
	  std::cerr << " => " << m->second->prev->second->key;
	std::cerr << std::endl;
      } 
    std::cerr << "\n" << std::endl;
#endif

    if (m_cache.size() > m_capacity)
      {
	// size_t ctr = 0;
	// size_t oldsize = m_cache.size();
	while (m_cache.size() > m_capacity && m_qfirst->second.use_count() == 1)
	  {
	    m_qfirst = m_qfirst->second->next;
	    // std::cerr << "erasing " << ++ctr << "/" << m_cache.size() << " " 
	    // << m_qfirst->second->key << std::endl;
	    m_cache.erase(m_qfirst->second->prev);
	  }
	// if (oldsize > m_cache.size()) std::cerr << "\n" << std::endl;
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

// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; cc-style: moses-cc-style -*-
#pragma once
#ifndef NO_MOSES
namespace sapt {

template<typename Token>
SPTR<pstats>
Bitext<Token>::
lookup(ttasksptr const& ttask, iter const& phrase, int max_sample) const
{
  SPTR<pstats> ret = prep2(ttask, phrase, max_sample);
  UTIL_THROW_IF2(!ret, "Got NULL pointer where I expected a valid pointer.");
  
  // Why were we locking here?
  if (m_num_workers <= 1)
    {
      boost::unique_lock<boost::shared_mutex> guard(m_lock);
      typename agenda::worker(*this->ag)();
    }
  else
    {
      boost::unique_lock<boost::mutex> lock(ret->lock);
      while (ret->in_progress)
	ret->ready.wait(lock);
    }
  return ret;
}


template<typename Token>
void
Bitext<Token>::
prep(ttasksptr const& ttask, iter const& phrase, bool const track_sids) const
{
  prep2(ttask, phrase, track_sids, m_default_sample_size);
}


// prep2 schedules a phrase for sampling, and returns immediately
// the member function lookup retrieves the respective pstats instance
// and waits until the sampling is finished before it returns.
// This allows sampling in the background
template<typename Token>
SPTR<pstats>
Bitext<Token>
::prep2
( ttasksptr const& ttask, iter const& phrase, bool const track_sids,
  int max_sample) const
{
  if (max_sample < 0) max_sample = m_default_sample_size;
  SPTR<SamplingBias> bias;
  SPTR<Moses::ContextScope> scope = ttask->GetScope();
  SPTR<ContextForQuery> context = scope->get<ContextForQuery>(this);
  if (context) bias = context->bias;
  SPTR<pstats::cache_t> cache;
  // - no caching for rare phrases and special requests (max_sample)
  //   (still need to test what a good caching threshold is ...)
  // - use the task-specific cache when there is a sampling bias
  if (max_sample == int(m_default_sample_size)
      && phrase.approxOccurrenceCount() > m_pstats_cache_threshold)
    {
      cache = (phrase.root == I1.get()
	       ? (bias ? context->cache1 : m_cache1)
	       : (bias ? context->cache2 : m_cache2));
    }
  SPTR<pstats> ret;
  SPTR<pstats> const* cached;
  
  if (cache && (cached = cache->get(phrase.getPid(), ret)) && *cached)
    return *cached;
  boost::unique_lock<boost::shared_mutex> guard(m_lock);
  if (!ag)
    {
      ag.reset(new agenda(*this));
      if (m_num_workers > 1)
	ag->add_workers(m_num_workers);
    }
  ret = ag->add_job(this, phrase, max_sample, bias, track_sids);
  if (cache) cache->set(phrase.getPid(),ret);
  UTIL_THROW_IF2(ret == NULL, "Couldn't schedule sampling job.");
  return ret;
}



}
#endif

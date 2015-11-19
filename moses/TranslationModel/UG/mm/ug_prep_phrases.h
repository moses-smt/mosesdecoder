// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
// Functions for multi-threaded pre-fetching of phrase table entries
// Author: Ulrich Germann

#include "moses/TranslationModel/UG/generic/threading/ug_thread_pool.h"
#include "moses/thread_safe_container.h"
#include "ug_bitext.h"
#include "ug_lru_cache.h"

namespace Moses {
namespace sapt { 

template<typename Token> // , typename BITEXT>
struct StatsCollector
{
  typedef    lru_cache::LRU_Cache< uint64_t, pstats  > hcache_t;
  typedef ThreadSafeContainer<uint64_t, SPTR<pstats> > pcache_t;
  typedef                 map<uint64_t, SPTR<pstats> > lcache_t;
  SPTR<Bitext<Token> const> bitext; // underlying bitext
  sampling_method           method; // sampling method 
  size_t               sample_size; // sample size 
  SPTR<SamplingBias const>    bias; // sampling bias
  hcache_t*                 hcache; // "history" cache
  pcache_t*                 pcache; // permanent cache
  size_t                 pcache_th; // threshold for adding items to pcache 
  SPTR<lcache_t>            lcache; // local cache
  ug::ThreadPool*            tpool; // thread pool to run jobs on 
  
  StatsCollector(SPTR<Bitext<Token> > xbitext, 
		 SPTR<SamplingBias> const xbias) 
    : method(ranked_sampling)
    , sample_size(100)
    , bias(xbias)
    , hcache(NULL)
    , pcache(NULL)
    , pcache_th(10000)
    , tpool(NULL)
  { 
    bitext = xbitext;
  }

  void
  process(typename TSA<Token>::tree_iterator& m, 
	  typename TSA<Token>::tree_iterator& r) 
  {
    if (!lcache) lcache.reset(new lcache_t);
    if (m.down())
      {
        do 
          {
            if (!r.extend(m.getToken(-1)->id())) continue;
            this->process(m, r);
            uint64_t pid = r.getPid();
            SPTR<pstats> stats;
            if (hcache) stats = hcache->get(pid); 
            if (!stats && pcache)
              {
                SPTR<pstats> const* foo = pcache->get(pid);
                if (foo) stats = *foo; 
              }
            if (!stats) // need to sample
              {
                BitextSampler<Token> s(bitext.get(), r, bias, sample_size, method);
                stats = s.stats();
                if (hcache) hcache->set(pid,stats);
                if (pcache && r.ca() >= pcache_th) pcache->set(pid,stats);
                if (tpool) tpool->add(s);
                else s();
              }
            (*lcache)[pid] = stats;
            r.up();
          }
        while (m.over());
        m.up();
      }
  }
};
} // end of namespace sapt
} // end of namespace Moses

#if 0
#endif
	    // r.up();

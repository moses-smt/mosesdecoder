// -*- c++ -*-
#pragma once

#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>

#include "ug_typedefs.h"
#include "ug_bitext_jstats.h"
#include "moses/thread_safe_container.h"

namespace Moses
{
  namespace bitext
  {
    struct
    pstats
    {
      typedef boost::unordered_map<uint64_t, sptr<pstats> > map_t;
      typedef ThreadSafeContainer<uint64_t, sptr<pstats>, map_t> cache_t;
      typedef std::vector<uchar> alnvec;
#if UG_BITEXT_TRACK_ACTIVE_THREADS
      static ThreadSafeCounter active;
#endif
      boost::mutex lock;               // for parallel gathering of stats
      boost::condition_variable ready; // consumers can wait for me to be ready

      size_t raw_cnt;     // (approximate) raw occurrence count
      size_t sample_cnt;  // number of instances selected during sampling
      size_t good;        // number of selected instances with valid word alignments
      size_t sum_pairs;   // total number of target phrases extracted (can be > raw_cnt)
      size_t in_progress; // how many threads are currently working on this?

      uint32_t ofwd[Moses::LRModel::NONE+1]; // distribution of fwd phrase orientations
      uint32_t obwd[Moses::LRModel::NONE+1]; // distribution of bwd phrase orientations

      std::vector<uint32_t> indoc; // distribution over where samples came from

      typedef std::map<uint64_t, jstats> trg_map_t;
      trg_map_t trg;
      pstats();
      ~pstats();
      void release();
      void register_worker();
      size_t count_workers() { return in_progress; }

      bool
      add(uint64_t const  pid, // target phrase id
	  float const       w, // sample weight (1./(# of phrases extractable))
	  alnvec const&     a, // local alignment
	  uint32_t const cnt2, // raw target phrase count
	  uint32_t fwd_o,      // fwd. phrase orientation
	  uint32_t bwd_o,      // bwd. phrase orientation
	  int const docid);    // document where sample was found

      void
      count_sample(int const docid,        // document where sample was found
		   size_t const num_pairs, // # of phrases extractable here
		   int const po_fwd,       // fwd phrase orientation
		   int const po_bwd);      // bwd phrase orientation
    };

  }
}

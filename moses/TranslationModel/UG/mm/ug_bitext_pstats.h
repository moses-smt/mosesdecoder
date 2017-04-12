// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>

#include "ug_typedefs.h"
#include "ug_bitext_jstats.h"
#include "moses/thread_safe_container.h"

namespace sapt
{
  struct
  pstats
  {
    typedef boost::unordered_map<uint64_t, SPTR<pstats> > map_t;
    typedef Moses::ThreadSafeContainer<uint64_t, SPTR<pstats>, map_t> cache_t;
    typedef std::vector<unsigned char> alnvec;
    typedef boost::unordered_map<uint64_t, jstats> trg_map_t;
    typedef boost::unordered_map<uint32_t,uint32_t> indoc_map_t; 
#if UG_BITEXT_TRACK_ACTIVE_THREADS
    static ThreadSafeCounter active;
#endif
    mutable boost::mutex lock;               // for parallel gathering of stats
    mutable boost::condition_variable ready; // consumers can wait for me to be ready

    size_t raw_cnt;     // (approximate) raw occurrence count
    size_t sample_cnt;  // number of instances selected during sampling
    size_t good;        // number of selected instances with valid word alignments
    size_t sum_pairs;   // total number of target phrases extracted (can be > raw_cnt)
    size_t in_progress; // how many threads are currently working on this?

    uint32_t ofwd[LRModel::NONE+1]; // distribution of fwd phrase orientations
    uint32_t obwd[LRModel::NONE+1]; // distribution of bwd phrase orientations

    indoc_map_t indoc;
    trg_map_t trg;
    bool track_sids;
    pstats(bool const track_sids);
    ~pstats();
    void release();
    void register_worker();
    size_t count_workers() { return in_progress; }

    size_t
    add(uint64_t const  pid, // target phrase id
        float const       w, // sample weight (1./(# of phrases extractable))
        float const       b, // sample bias score
        alnvec const&     a, // local alignment
        uint32_t const cnt2, // raw target phrase count
        uint32_t fwd_o,      // fwd. phrase orientation
        uint32_t bwd_o,      // bwd. phrase orientation
        int const docid,     // document where sample was found
        uint32_t const sid); // index of sentence where sample was found
    
    void
    count_sample(int const docid,        // document where sample was found
		 size_t const num_pairs, // # of phrases extractable here
		 int const po_fwd,       // fwd phrase orientation
		 int const po_bwd);      // bwd phrase orientation
    void wait() const;
  };

}


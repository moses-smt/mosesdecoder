#ifndef __ug_mm_bitext_h
#define __ug_mm_bitext_h
// Memory-mapped, word-aligned bitext
// Written by Ulrich Germann

// things we can do to speed up things:
// - set up threads at startup time that force the 
//   data in to memory sequentially
//
// - use multiple agendas for better load balancing and to avoid 
//   competition for locks

#include <string>
#include <vector>
#include <cassert>
#include <iomanip>
#include <algorithm>

#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include "moses/generic/sorting/VectorIndexSorter.h"
#include "moses/generic/sampling/Sampling.h"
#include "moses/generic/file_io/ug_stream.h"

#include "ug_typedefs.h"
#include "ug_mm_ttrack.h"
#include "ug_mm_tsa.h"
#include "tpt_tokenindex.h"
#include "ug_corpus_token.h"
#include "tpt_pickler.h"

using namespace ugdiss;
using namespace std;
namespace Moses {

  typedef L2R_Token<SimpleWordId>      Token;
  typedef mmTSA<Token>::tree_iterator   iter;

  class mmbitext
  {
  public:
    typedef mmTSA<Token>::tree_iterator iter;
    class pstats; // one-sided phrase statistics
    class jstats; // phrase pair ("joint") statistics
    class agenda
    {
      boost::mutex               lock; 
      boost::condition_variable ready; 
      class job;
      class worker;
      list<job> joblist;
      vector<sptr<boost::thread> > workers;
      bool shutdown;
      size_t doomed;
    public:
      mmbitext const& bitext;
      agenda(mmbitext const& bitext);
      ~agenda();
      void add_workers(int n);
      sptr<pstats> add_job(mmbitext::iter const& phrase, 
			   size_t const max_samples);
      bool get_task(uint64_t & sid, uint64_t & offset, uint64_t & len, 
		    bool & fwd, sptr<mmbitext::pstats> & stats);
    };

    // stores the list of unfinished jobs;
    // maintains a pool of workers and assigns the jobs to them

    agenda* ag;
    mmTtrack<char>  Tx;    // word alignments
    mmTtrack<Token> T1,T2; // token tracks
    TokenIndex      V1,V2; // vocabs
    mmTSA<Token>    I1,I2; // suffix arrays 

    /// given the source phrase sid[start:stop]
    //  find the possible start (s1 .. s2) and end (e1 .. e2) 
    //  points of the target phrase; if non-NULL, store word
    //  alignments in *core_alignment. If /flip/, source phrase is 
    //  L2.
    bool 
    find_trg_phr_bounds
    (size_t const sid, size_t const start, size_t const stop, 
     size_t & s1, size_t & s2, size_t & e1, size_t & e2, 
     vector<uchar> * core_alignment, bool const flip) const;

    boost::unordered_map<uint64_t,sptr<pstats> > cache1,cache2;
  private:
    sptr<pstats> 
    prep2(iter const& phrase);
  public:
    mmbitext();
    ~mmbitext();

    void open(string const base, string const L1, string const L2);

    sptr<pstats> lookup(iter const& phrase);
    void prep(iter const& phrase);
  };

  // "joint" (i.e., phrase pair) statistics
  class
  mmbitext::
  jstats
  {
    uint32_t my_rcnt; // unweighted count
    float    my_wcnt; // weighted count 
    vector<pair<size_t, vector<uchar> > > my_aln; 
    boost::mutex lock;
  public:
    jstats();
    jstats(jstats const& other);
    uint32_t rcnt() const;
    float    wcnt() const;
    vector<pair<size_t, vector<uchar> > > const & aln() const;
    void add(float w, vector<uchar> const& a);
  };

  // struct 
  // mmbitext:
  // phrasepair
  // {
  //   Token const* t;
  //   size_t len;
  //   size_t cnt;
  //   float  fwd, bwd;
  
  //   map<uint32_t,uint32_t> aln;
  //   string toString(TokenIndex const& V) const;
  //   bool operator<(phrase const& other) const;
  //   bool operator>(phrase const& other) const;
  //   phrase(pair<pair<Token const*, size_t>,jstats> const & foo);
    
  // };


  struct 
  mmbitext::
  pstats
  {
    boost::mutex lock; // for parallel gathering of stats
    boost::condition_variable ready; // consumers can wait for this data structure to be ready.

    size_t raw_cnt;    // (approximate) raw occurrence count 
    size_t sample_cnt; // number of instances selected during sampling
    size_t good;       // number of selected instances with valid word alignments
    size_t sum_pairs;
    // size_t snt_cnt; 
    // size_t sample_snt;
    size_t in_progress; // keeps track of how many threads are currently working on this
    boost::unordered_map<uint64_t, jstats> trg;
    pstats(); 
    // vector<phrase> nbest;
    // void select_nbest(size_t const N=10);
    void release();
    void register_worker();
    void add(mmbitext::iter const& trg_phrase, float const w, vector<uchar> const& a);
  };

  class
  mmbitext::
  agenda::
  worker
  {
    agenda& ag;
  public:
    worker(agenda& a);
    void operator()();
    
  };

  class
  mmbitext::
  agenda::
  job
  {
  public:
    char const*   next;
    char const*   stop;
    size_t max_samples;
    size_t         ctr;
    size_t         len;
    bool           fwd;
    sptr<mmbitext::pstats> stats;
    bool step(uint64_t & sid, uint64_t & offset);
  };

}
#endif


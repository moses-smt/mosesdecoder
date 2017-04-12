// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once
// Implementations of word-aligned bitext.
// Written by Ulrich Germann
//
// mmBitext: static, memory-mapped bitext
// imBitext: dynamic, in-memory bitext
//

// things we can do to speed up things:
// - set up threads at startup time that force the
//   data in to memory sequentially
//
// - use multiple agendas for better load balancing and to avoid
//   competition for locks
//


#define UG_BITEXT_TRACK_ACTIVE_THREADS 0

#include <string>
#include <vector>
#include <cassert>
#include <iomanip>
#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <boost/math/distributions/binomial.hpp>

#include "moses/TranslationModel/UG/generic/sorting/VectorIndexSorter.h"
#include "moses/TranslationModel/UG/generic/sampling/Sampling.h"
#include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"
#include "moses/TranslationModel/UG/generic/threading/ug_thread_safe_counter.h"
#include "moses/TranslationModel/UG/generic/threading/ug_ref_counter.h"
// #include "moses/FF/LexicalReordering/LexicalReorderingState.h"
#include "moses/Util.h"

#ifndef NO_MOSES
// #pragma message "COMPILING WITH MOSES SUPPORT!"
#include "moses/StaticData.h"
#include "moses/thread_safe_container.h"
#include "moses/ContextScope.h"
#include "moses/TranslationTask.h"
#else
// #pragma message "COMPILING WITHOUT MOSES SUPPORT!"
#endif

#include "util/exception.hh"
// #include "util/check.hh"

#include "ug_typedefs.h"
#include "ug_mm_ttrack.h"
#include "ug_im_ttrack.h"
#include "ug_mm_tsa.h"
#include "ug_im_tsa.h"
#include "tpt_tokenindex.h"
#include "ug_corpus_token.h"
#include "tpt_pickler.h"
#include "ug_lexical_phrase_scorer2.h"
#include "ug_lru_cache.h"
#include "ug_lexical_reordering.h"
#include "ug_sampling_bias.h"
#include "ug_phrasepair.h"
#include "ug_bitext_phrase_extraction_record.h"
#include "moses/TranslationModel/UG/generic/threading/ug_ref_counter.h"

// Minimum source count for caching phrase lookup statistics.
// If source phrase occurs less frequently, never cache; 
// always re-compute.
#define PSTATS_CACHE_THRESHOLD 50

namespace Moses { class Mmsapt; }
namespace sapt
{
  using Moses::ttasksptr;
  using Moses::ttaskwptr;
  using tpt::binread;
  using tpt::binwrite;

  float lbop(size_t const tries, size_t const succ, float const confidence);
  void write_bitvector(bitvector const& v, std::ostream& out);

#ifndef NO_MOSES
  struct
  ContextForQuery
  {
    // needs to be made thread-safe
    // ttasksptr const m_ttask;
    // size_t max_samples;
    boost::shared_mutex lock;
    SPTR<SamplingBias> bias;
    SPTR<pstats::cache_t> cache1, cache2;
    std::ostream* bias_log;
    ContextForQuery() : bias_log(NULL) { }
  };
#endif

  template<typename Token> class BitextSampler;
  
  template<typename TKN>
  class Bitext // : public Moses::reference_counter
  {
  public:
    template<typename Token> friend class BitextSampler;
    typedef TKN Token;
    typedef typename TSA<Token>::tree_iterator   iter;
    typedef typename std::vector<PhrasePair<Token> > vec_ppair;
    typedef typename lru_cache::LRU_Cache<uint64_t, vec_ppair> pplist_cache_t;
    typedef TSA<Token> tsa;
    friend class Moses::Mmsapt;
  protected:
    mutable boost::shared_mutex m_lock; // for thread-safe operation

    class agenda; // for parallel sampling see ug_bitext_agenda.h
    mutable SPTR<agenda> ag;
    size_t m_num_workers; // number of workers available to the agenda

    size_t m_default_sample_size;
    size_t m_pstats_cache_threshold; // threshold for caching sampling results
    SPTR<pstats::cache_t> m_cache1, m_cache2; // caches for sampling results

    std::vector<std::string> m_docname;
    std::map<std::string,id_type>  m_docname2docid; // maps from doc names to ids
    SPTR<std::vector<id_type> >   m_sid2docid; // maps from sentences to docs (ids)

    mutable pplist_cache_t m_pplist_cache1, m_pplist_cache2;
    // caches for unbiased sampling; biased sampling uses the caches that
    // are stored locally on the translation task
  public:
    SPTR<Ttrack<char> >  Tx; // word alignments
    SPTR<Ttrack<Token> > T1; // token track
    SPTR<Ttrack<Token> > T2; // token track
    SPTR<TokenIndex>     V1; // vocab
    SPTR<TokenIndex>     V2; // vocab
    SPTR<TSA<Token> >    I1; // indices
    SPTR<TSA<Token> >    I2; // indices

    /// given the source phrase sid[start:stop]
    //  find the possible start (s1 .. s2) and end (e1 .. e2)
    //  points of the target phrase; if non-NULL, store word
    //  alignments in *core_alignment. If /flip/, source phrase is
    //  L2.
    bool find_trg_phr_bounds(PhraseExtractionRecord& rec) const;
    bool find_trg_phr_bounds
    ( size_t const sid,    // sentence to investigate
      size_t const start,  // start of source phrase
      size_t const stop,   // last position of source phrase
      size_t & s1, size_t & s2, // beginning and end of target start
      size_t & e1, size_t & e2, // beginning and end of target end
      int& po_fwd, int& po_bwd, // phrase orientations
      std::vector<unsigned char> * core_alignment, // stores the core alignment
      bitvector* full_alignment, // stores full word alignment for this sent.
      bool const flip) const;   // flip source and target (reverse lookup)

    // prep2 launches sampling and returns immediately.
    // lookup (below) waits for the job to finish before it returns
    SPTR<pstats>
    prep2(iter const& phrase, int max_sample = -1) const;

#ifndef NO_MOSES
    SPTR<pstats>
    prep2(ttasksptr const& ttask, iter const& phrase, bool const track_sids,
          int max_sample = -1) const;
#endif 

  protected:
    Bitext(size_t const max_sample = 1000, size_t const xnum_workers = 16);

    Bitext(Ttrack<Token>* const t1, Ttrack<Token>* const t2,
           Ttrack<char>*  const tx,
           TokenIndex*    const v1, TokenIndex*    const v2,
           TSA<Token>*    const i1, TSA<Token>*    const i2,
           size_t const max_sample=1000,
           size_t const xnum_workers=16);
  public:
    virtual void
    open(std::string const base, std::string const L1, std::string const L2) = 0;

    SPTR<pstats> 
    lookup(iter const& phrase, int max_sample = -1) const;

    void prep(iter const& phrase) const;

#ifndef NO_MOSES
    SPTR<pstats>
    lookup(ttasksptr const& ttask, iter const& phrase, int max_sample = -1) const;

    void prep(ttasksptr const& ttask, iter const& phrase, bool const track_sids) const;
#endif

    void   setDefaultSampleSize(size_t const max_samples);
    size_t getDefaultSampleSize() const;

    std::string toString(uint64_t pid, int isL2) const;

    virtual size_t revision() const { return 0; }

    SPTR<SentenceBias>
    loadSentenceBias(std::string const& fname) const;

    SPTR<DocumentBias>
    SetupDocumentBias(std::string const& bserver, std::string const& text, 
                      std::ostream* log) const;

    SPTR<DocumentBias>
    SetupDocumentBias(std::map<std::string,float> context_weights, 
                      std::ostream* log) const;

    void
    mark_match(Token const* start, Token const* end, iter const& m,
               bitvector& check) const;
    void
    write_yawat_alignment
    ( id_type const sid, iter const* m1, iter const* m2, std::ostream& out ) const;

    std::string sid2docname(id_type const sid) const;
    std::string docid2name(id_type const sid) const;
    int docname2docid(std::string const& name) const;
    
    std::vector<id_type> const* sid2did() const;
    int sid2did(uint32_t sid) const;
  };

  #include "ug_bitext_agenda.h"

  template<typename Token>
  int
  Bitext<Token>::
  docname2docid(std::string const& name) const
  {
    std::map<std::string,id_type>::const_iterator m;
    m = m_docname2docid.find(name);
    if (m != m_docname2docid.end()) return m->second;
    return -1;
  }

  template<typename Token>
  std::string
  Bitext<Token>::
  docid2name(id_type const did) const
  {
    if (did < m_docname.size())
      return m_docname[did];
    else
      return (boost::format("%d") % did).str();
  }

  template<typename Token>
  std::string
  Bitext<Token>::
  sid2docname(id_type const sid) const
  {
    if (sid < m_sid2docid->size() && (*m_sid2docid)[sid] < m_docname.size())
      return m_docname[(*m_sid2docid)[sid]];
    else
      return "";
  }

  template<typename Token>
  std::vector<id_type> const*
  Bitext<Token>::
  sid2did() const
  {
    return m_sid2docid.get();
  }

  template<typename Token>
  int
  Bitext<Token>::
  sid2did(uint32_t sid) const
  {
    if (m_sid2docid) 
      return m_sid2docid->at(sid);
    return -1;
  }


  template<typename Token>
  SPTR<SentenceBias>
  Bitext<Token>::
  loadSentenceBias(std::string const& fname) const
  {
    SPTR<SentenceBias> ret(new SentenceBias(T1->size()));
    std::ifstream in(fname.c_str());
    size_t i = 0;
    float v; while (in>>v) (*ret)[i++] = v;
    UTIL_THROW_IF2(i != T1->size(),
                   "Mismatch between bias vector size and corpus size at "
                   << HERE);
    return ret;
  }

  template<typename Token>
  std::string
  Bitext<Token>::
  toString(uint64_t pid, int isL2) const
  {
    std::ostringstream buf;
    uint32_t sid,off,len; parse_pid(pid,sid,off,len);
    Token const* t = (isL2 ? T2 : T1)->sntStart(sid) + off;
    Token const* x = t + len;
    TokenIndex const& V = isL2 ? *V2 : *V1;
    while (t < x)
      {
        buf << V[t->id()];
        if (++t < x) buf << " ";
      }
    return buf.str();
  }

  template<typename Token>
  size_t
  Bitext<Token>::
  getDefaultSampleSize() const
  {
    return m_default_sample_size;
  }
  template<typename Token>
  void
  Bitext<Token>::
  setDefaultSampleSize(size_t const max_samples)
  {
    boost::unique_lock<boost::shared_mutex> guard(m_lock);
    if (max_samples != m_default_sample_size)
      {
        m_cache1.reset(new pstats::cache_t);
        m_cache2.reset(new pstats::cache_t);
        m_default_sample_size = max_samples;
      }
  }

  template<typename Token>
  Bitext<Token>::
  Bitext(size_t const max_sample, size_t const xnum_workers)
    : m_num_workers(xnum_workers)
    , m_default_sample_size(max_sample)
    , m_pstats_cache_threshold(PSTATS_CACHE_THRESHOLD)
    , m_cache1(new pstats::cache_t)
    , m_cache2(new pstats::cache_t)
  { }

  template<typename Token>
  Bitext<Token>::
  Bitext(Ttrack<Token>* const t1,
         Ttrack<Token>* const t2,
         Ttrack<char>*  const tx,
         TokenIndex*    const v1,
         TokenIndex*    const v2,
         TSA<Token>* const i1,
         TSA<Token>* const i2,
         size_t const max_sample,
         size_t const xnum_workers)
    : m_num_workers(xnum_workers)
    , m_default_sample_size(max_sample)
    , m_pstats_cache_threshold(PSTATS_CACHE_THRESHOLD)
    , m_cache1(new pstats::cache_t)
    , m_cache2(new pstats::cache_t)
    , Tx(tx), T1(t1), T2(t2), V1(v1), V2(v2), I1(i1), I2(i2)
  { }

  template<typename TKN> class snt_adder;
  template<>             class snt_adder<L2R_Token<SimpleWordId> >;

  template<>
  class snt_adder<L2R_Token<SimpleWordId> >
  {
    typedef L2R_Token<SimpleWordId> TKN;
    std::vector<std::string> const & snt;
    TokenIndex           & V;
    SPTR<imTtrack<TKN> > & track;
    SPTR<imTSA<TKN > >   & index;
  public:
    snt_adder(std::vector<std::string> const& s, TokenIndex& v,
              SPTR<imTtrack<TKN> >& t, SPTR<imTSA<TKN> >& i);

    void operator()();
  };

  template<typename Token>
  bool
  Bitext<Token>::
  find_trg_phr_bounds(PhraseExtractionRecord& rec) const
  {
    return find_trg_phr_bounds(rec.sid, rec.start, rec.stop,
                               rec.s1, rec.s2, rec.e1, rec.e2,
                               rec.po_fwd, rec.po_bwd, 
                               rec.aln, rec.full_aln, rec.flip);
  }

  template<typename Token>
  bool
  Bitext<Token>::
  find_trg_phr_bounds
  ( size_t const sid,    // sentence to investigate
    size_t const start,  // start of source phrase
    size_t const stop,   // last position of source phrase
    size_t & s1, size_t & s2, // beginning and end of target start
    size_t & e1, size_t & e2, // beginning and end of target end
    int& po_fwd, int& po_bwd, // phrase orientations
    std::vector<unsigned char> * core_alignment, // stores the core alignment
    bitvector* full_alignment, // stores full word alignment for this sent.
    bool const flip) const     // flip source and target (reverse lookup)
  {
    // if (core_alignment) cout << "HAVE CORE ALIGNMENT" << endl;
    // a word on the core_alignment (core_alignment):
    //
    // Since fringe words ([s1,...,s2),[e1,..,e2) if s1 < s2, or e1
    // < e2, respectively) are be definition unaligned, we store
    // only the core alignment in *aln. It is up to the calling
    // function to shift alignment points over for start positions
    // of extracted phrases that start with a fringe word
    assert(T1);
    assert(T2);
    assert(Tx);

    size_t slen1,slen2;
    if (flip)
      {
        slen1 = T2->sntLen(sid);
        slen2 = T1->sntLen(sid);
      }
    else
      {
        slen1 = T1->sntLen(sid);
        slen2 = T2->sntLen(sid);
      }
    bitvector forbidden(slen2);
    if (full_alignment)
      {
        if (slen1*slen2 > full_alignment->size())
          full_alignment->resize(slen1*slen2*2);
        full_alignment->reset();
      }
    size_t src,trg;
    size_t lft = forbidden.size();
    size_t rgt = 0;
    std::vector<std::vector<ushort> > aln1(slen1),aln2(slen2);

    // process word alignment for this sentence
    char const* p = Tx->sntStart(sid);
    char const* x = Tx->sntEnd(sid);
    while (p < x)
      {
        if (flip) 
          { 
            p = binread(p,trg); 
            assert(p<x); 
            p = binread(p,src); 
          }
        else 
          { 
            p = binread(p,src); 
            assert(p<x); 
            p = binread(p,trg); 
          }
	  
        UTIL_THROW_IF2((src >= slen1 || trg >= slen2),
                       "Alignment range error at sentence " << sid << "!\n"
                       << src << "/" << slen1 << " " << trg << "/" << slen2);
	  
        if (src < start || src >= stop)
          forbidden.set(trg);
        else
          {
            lft = std::min(lft,trg);
            rgt = std::max(rgt,trg);
          }
        if (core_alignment)
          {
            aln1[src].push_back(trg);
            aln2[trg].push_back(src);
          }
        if (full_alignment)
          full_alignment->set(src*slen2 + trg);
      }

    for (size_t i = lft; i <= rgt; ++i)
      if (forbidden[i])
        return false;

    s2 = lft;   for (s1 = s2; s1 && !forbidden[s1-1]; --s1);
    e1 = rgt+1; for (e2 = e1; e2 < forbidden.size() && !forbidden[e2]; ++e2);

    if (lft > rgt) return false;
    if (core_alignment)
      {
        core_alignment->clear();
        for (size_t i = start; i < stop; ++i)
          {
            BOOST_FOREACH(ushort x, aln1[i])
              {
                core_alignment->push_back(i - start);
                core_alignment->push_back(x - lft);
              }
          }
        // now determine fwd and bwd phrase orientation
        po_fwd = find_po_fwd(aln1,aln2,start,stop,s1,e2);
        po_bwd = find_po_bwd(aln1,aln2,start,stop,s1,e2);
      }
    return lft <= rgt;
  }

  template<typename Token>
  SPTR<DocumentBias>
  Bitext<Token>::
  SetupDocumentBias
  ( std::string const& bserver, std::string const& text, std::ostream* log ) const
  {
    SPTR<DocumentBias> ret;
    UTIL_THROW_IF2(m_sid2docid == NULL,
                   "Document bias requested but no document map loaded.");
    ret.reset(new DocumentBias(*m_sid2docid, m_docname2docid,
                               bserver, text, log));
    return ret;
  }

  template<typename Token>
  SPTR<DocumentBias>
  Bitext<Token>::
  SetupDocumentBias
  ( std::map<std::string,float> context_weights, std::ostream* log ) const
  {
    SPTR<DocumentBias> ret;
    UTIL_THROW_IF2(m_sid2docid == NULL,
                   "Document bias requested but no document map loaded.");
    ret.reset(new DocumentBias(*m_sid2docid, m_docname2docid,
                               context_weights, log));
    return ret;
  }

  template<typename Token>
  void
  Bitext<Token>::
  prep(iter const& phrase) const
  {
    prep2(phrase, m_default_sample_size);
  }



  // prep2 schedules a phrase for sampling, and returns immediately
  // the member function lookup retrieves the respective pstats instance
  // and waits until the sampling is finished before it returns.
  // This allows sampling in the background
  template<typename Token>
  SPTR<pstats>
  Bitext<Token>
  ::prep2
  (iter const& phrase, int max_sample) const
  {
    if (max_sample < 0) max_sample = m_default_sample_size;
    SPTR<SamplingBias> bias;
    SPTR<pstats::cache_t> cache;
    // - no caching for rare phrases and special requests (max_sample)
    //   (still need to test what a good caching threshold is ...)
    // - use the task-specific cache when there is a sampling bias
    if (max_sample == int(m_default_sample_size)
        && phrase.approxOccurrenceCount() > m_pstats_cache_threshold)
      {
        cache = (phrase.root == I1.get() ? m_cache1 : m_cache2);
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
    ret = ag->add_job(this, phrase, max_sample, bias);
    if (cache) cache->set(phrase.getPid(),ret);
    UTIL_THROW_IF2(ret == NULL, "Couldn't schedule sampling job.");
    return ret;
  }

  // worker for scoring and sorting phrase table entries in parallel
  template<typename Token>
  class pstats2pplist
  {
    Ttrack<Token> const& m_other;
    SPTR<pstats> m_pstats;
    std::vector<PhrasePair<Token> >& m_pplist;
    typename PhrasePair<Token>::Scorer const* m_scorer;
    PhrasePair<Token> m_pp;
    Token const* m_token;
    size_t m_len;
    uint64_t m_pid1;
    bool m_is_inverse;
  public:

    // CONSTRUCTOR
    pstats2pplist(typename TSA<Token>::tree_iterator const& m,
                  Ttrack<Token> const& other,
                  SPTR<pstats> const& ps,
                  std::vector<PhrasePair<Token> >& dest,
                  typename PhrasePair<Token>::Scorer const* scorer)
      : m_other(other)
      , m_pstats(ps)
      , m_pplist(dest)
      , m_scorer(scorer)
      , m_token(m.getToken(0))
      , m_len(m.size())
      , m_pid1(m.getPid())
      , m_is_inverse(false)
    { }

    // WORKER
    void
    operator()()
    {
      // wait till all statistics have been collected
      boost::unique_lock<boost::mutex> lock(m_pstats->lock);
      while (m_pstats->in_progress)
        m_pstats->ready.wait(lock);

      m_pp.init(m_pid1, m_is_inverse, m_token,m_len,m_pstats.get(),0);

      // convert pstats entries to phrase pairs
      pstats::trg_map_t::iterator a;
      for (a = m_pstats->trg.begin(); a != m_pstats->trg.end(); ++a)
        {
          uint32_t sid,off,len;
          parse_pid(a->first, sid, off, len);
          m_pp.update(a->first, m_other.sntStart(sid)+off, len, a->second);
          m_pp.good2 = max(uint32_t(m_pp.raw2 * float(m_pp.good1)/m_pp.raw1),
                           m_pp.joint);
          // Poor man's early pruning: if p(f|e) or p(e|f) < 1/128, don't
          // even consider the phrase pair, as it is unlikely to ever be 
          // considered as a valid translation. 
          size_t J = m_pp.joint<<7; // hard coded threshold of 1/128
          if (m_pp.good1 > J || m_pp.good2 > J) continue;
          if (m_scorer)
            {
              (*m_scorer)(m_pp);
            }
          m_pplist.push_back(m_pp);
        }
      std::greater<PhrasePair<Token> > sorter;
      if (m_scorer) sort(m_pplist.begin(), m_pplist.end(),sorter);
    }
  };

  template<typename Token>
  void
  Bitext<Token>
  ::mark_match(Token const* start, Token const* end,
               iter const& m, bitvector& check) const
  {
    check.resize(end-start);
    check.reset();
    Token const* x = m.getToken(0);
    for (Token const* s = start; s < end; ++s)
      {
        if (s->id() != x->id()) continue;
        Token const* a = x;
        Token const* b = s;
        size_t i = 0;
        while (a && b && a->id() == b->id() && i < m.size())
          {
            ++i;
            a = a->next();
            b = b->next();
          }
        if (i == m.size())
          {
            b = s;
            while (i-- > 0) { check.set(b-start); b = b->next(); }
          }
      }
  }

  template<typename Token>
  void
  Bitext<Token>::
  write_yawat_alignment
  ( id_type const sid, iter const* m1, iter const* m2, std::ostream& out ) const
  {
    std::vector<int> a1(T1->sntLen(sid),-1), a2(T2->sntLen(sid),-1);
    bitvector f1(a1.size()), f2(a2.size());
    if (m1) mark_match(T1->sntStart(sid), T1->sntEnd(sid), *m1, f1);
    if (m2) mark_match(T2->sntStart(sid), T2->sntEnd(sid), *m2, f2);

    std::vector<std::pair<bitvector, bitvector> > agroups;
    std::vector<std::string> grouplabel;
    std::pair<bitvector, bitvector> ag;
    ag.first.resize(a1.size());
    ag.second.resize(a2.size());
    char const* x = Tx->sntStart(sid);
    size_t a, b;
    while (x < Tx->sntEnd(sid))
      {
        x = binread(x,a);
        x = binread(x,b);
        if (a1.at(a) < 0 && a2.at(b) < 0)
          {
            a1[a] = a2[b] = agroups.size();
            ag.first.reset();
            ag.second.reset();
            ag.first.set(a);
            ag.second.set(b);
            agroups.push_back(ag);
            grouplabel.push_back(f1[a] || f2[b] ? "infocusbi" : "unspec");
          }
        else if (a1.at(a) < 0)
          {
            a1[a] = a2[b];
            agroups[a2[b]].first.set(a);
            if (f1[a] || f2[b]) grouplabel[a1[a]] = "infocusbi";
          }
        else if (a2.at(b) < 0)
          {
            a2[b] = a1[a];
            agroups[a1[a]].second.set(b);
            if (f1[a] || f2[b]) grouplabel[a1[a]] = "infocusbi";
          }
        else
          {
            agroups[a1[a]].first  |= agroups[a2[b]].first;
            agroups[a1[a]].second |= agroups[a2[b]].second;
            a2[b] = a1[a];
            if (f1[a] || f2[b]) grouplabel[a1[a]] = "infocusbi";
          }
      }

    for (a = 0; a < a1.size(); ++a)
      {
        if (a1[a] < 0)
          {
            if (f1[a]) out << a << "::" << "infocusmono ";
            continue;
          }
        bitvector const& A = agroups[a1[a]].first;
        bitvector const& B = agroups[a1[a]].second;
        if (A.find_first() < a) continue;
        write_bitvector(A,out); out << ":";
        write_bitvector(B,out); out << ":";
        out << grouplabel[a1[a]] << " ";
      }
    for (b = 0; b < a2.size(); ++b)
      {
        if (a2[b] < 0 && f2[b])
          out <<  "::" << "infocusmono ";
      }
  }

  template<typename Token>
  void
  expand(typename Bitext<Token>::iter const& m,
         Bitext<Token> const& bt, pstats const& ps,
         std::vector<PhrasePair<Token> >& dest, std::ostream* log)
  {
    bool fwd = m.root == bt.I1.get();
    dest.reserve(ps.trg.size());
    PhrasePair<Token> pp;
    pp.init(m.getPid(), !fwd, m.getToken(0), m.size(), &ps, 0);
    // cout << HERE << " "
    // << toString(*(fwd ? bt.V1 : bt.V2), pp.start1,pp.len1) << std::endl;
    pstats::trg_map_t::const_iterator a;
    for (a = ps.trg.begin(); a != ps.trg.end(); ++a)
      {
        uint32_t sid,off,len;
        parse_pid(a->first, sid, off, len);
        pp.update(a->first, (fwd ? bt.T2 : bt.T1)->sntStart(sid)+off,
                  len, a->second);
        dest.push_back(pp);
      }
  }

} // end of namespace sapt

#include "ug_im_bitext.h"
#include "ug_mm_bitext.h"
#include "ug_bitext_moses.h"

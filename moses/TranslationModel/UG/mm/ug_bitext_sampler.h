// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil -*-
#pragma once
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/intrusive_ptr.hpp>
#include "ug_bitext.h"
#include "ug_bitext_pstats.h"
#include "ug_sampling_bias.h"
#include "ug_tsa_array_entry.h"
#include "ug_bitext_phrase_extraction_record.h"
#include "moses/TranslationModel/UG/generic/threading/ug_ref_counter.h"
#include "moses/TranslationModel/UG/generic/threading/ug_thread_safe_counter.h"
#include "moses/TranslationModel/UG/generic/sorting/NBestList.h"
namespace Moses
{
namespace bitext 
{
  
  enum sampling_method { full_coverage, random_sampling, ranked_sampling };

  typedef ugdiss::ttrack::Position TokenPosition;
  class CandidateSorter
  {
    SamplingBias const& score;
  public:
    CandidateSorter(SamplingBias const& s) : score(s) {}
    bool operator()(TokenPosition const& a, TokenPosition const& b) const
    { return score[a.sid] > score[b.sid]; }
  };
  
  template<typename Token>
  class
  BitextSampler : public reference_counter
  {
    typedef Bitext<Token> bitext;
    typedef TSA<Token>       tsa;
    typedef SamplingBias    bias;
    typedef typename Bitext<Token>::iter tsa_iter;
    mutable boost::condition_variable   m_ready; 
    mutable boost::mutex                 m_lock; 
    // const members
    // sptr<bitext const> const   m_bitext; // keep bitext alive while I am 
    // should be an 
    iptr<bitext const> const   m_bitext; // keep bitext alive as long as I am 
    size_t             const     m_plen; // length of lookup phrase
    bool               const      m_fwd; // forward or backward direction?
    sptr<tsa const>    const     m_root; // root of suffix array
    char               const*    m_next; // current position
    char               const*    m_stop; // end of search range
    sampling_method    const   m_method; /* look at all / random sample / 
					      * ranked samples */
    sptr<bias const>   const     m_bias; // bias over candidates
    size_t             const  m_samples; // how many samples at most 
    // non-const members
    sptr<pstats>                m_stats; // destination for phrase stats
    size_t                        m_ctr; // number of samples considered
    float                  m_total_bias; // for random sampling with bias
    bool                     m_finished;
    void   consider_sample(TokenPosition const& p);
    size_t perform_ranked_sampling();
    
  public:
    BitextSampler(BitextSampler const& other);
    BitextSampler const& operator=(BitextSampler const& other);
    BitextSampler(bitext const*  const bitext, typename bitext::iter const& phrase,
		  sptr<SamplingBias const> const& bias, size_t const max_samples,
		  sampling_method const method); 
    ~BitextSampler();
    bool operator()(); // run sampling
    sptr<pstats> stats();
    bool done() const;
  };
  
  template<typename Token>
  BitextSampler<Token>::
  BitextSampler(Bitext<Token> const* const bitext, 
		typename bitext::iter const& phrase,
		sptr<SamplingBias const> const& bias, size_t const max_samples,
		sampling_method const method)
    : m_bitext(bitext)
    , m_plen(phrase.size())
    , m_fwd(phrase.root == bitext->I1.get())
    , m_root(m_fwd ? bitext->I1 : bitext->I2)
    , m_next(phrase.lower_bound(-1))
    , m_stop(phrase.upper_bound(-1))
    , m_method(method)
    , m_bias(bias)
    , m_samples(max_samples)
    , m_ctr(0)
    , m_total_bias(0)
    , m_finished(false)
  {
    m_stats.reset(new pstats);
    m_stats->raw_cnt = phrase.ca();
    m_stats->register_worker();
  }
  
  template<typename Token>
  BitextSampler<Token>::
  BitextSampler(BitextSampler const& other)
    : m_bitext(other.m_bitext)
    , m_plen(other.m_plen)
    , m_fwd(other.m_fwd)
    , m_root(other.m_root)
    , m_next(other.m_next)
    , m_stop(other.m_stop)
    , m_method(other.m_method)
    , m_bias(other.m_bias)
    , m_samples(other.m_samples)
  {
    // lock both instances
    boost::unique_lock<boost::mutex> mylock(m_lock);
    boost::unique_lock<boost::mutex> yrlock(other.m_lock);
    // actually, BitextSamplers should only copied on job submission
    m_stats = other.m_stats;
    m_stats->register_worker();
    m_ctr = other.m_ctr; 
    m_total_bias = other.m_total_bias;
    m_finished = other.m_finished;
  }

  // Ranked sampling sorts all samples by score and then considers the top-ranked 
  // candidates for phrase extraction.
  template<typename Token>
  size_t
  BitextSampler<Token>::
  perform_ranked_sampling()
  {
    if (m_next == m_stop) return m_ctr;
    CandidateSorter sorter(*m_bias);
    NBestList<TokenPosition, CandidateSorter> nbest(m_samples,sorter);
    ugdiss::tsa::ArrayEntry I(m_next);
    while (I.next < m_stop)
      {
        ++m_ctr;
        nbest.add(m_root->readEntry(I.next,I));
      }
    for (size_t i = 0; i < nbest.size(); ++i)
      consider_sample(nbest.get_unsorted(i));
    // cerr << m_ctr << " samples considered at " 
    //      << __FILE__ << ":" << __LINE__ << endl;
    return m_ctr;
  }
  
  template<typename Token>
  void
  BitextSampler<Token>::
  consider_sample(TokenPosition const& p)
  {
    vector<uchar>  aln; 
    bitvector full_aln(100*100);
    PhraseExtractionRecord rec(p.sid, p.offset, p.offset + m_plen, 
                               !m_fwd, &aln, &full_aln);
    int docid  = m_bias ? m_bias->GetClass(p.sid) : -1;
    bool good = m_bitext->find_trg_phr_bounds(rec);
    if (!good)
      { // no good, probably because phrase is not coherent
        m_stats->count_sample(docid, 0, rec.po_fwd, rec.po_bwd);
        return;
      }
    
    // all good: register this sample as valid
    size_t num_pairs = (rec.s2 - rec.s1 + 1) * (rec.e2 - rec.e1 + 1);
    m_stats->count_sample(docid, num_pairs, rec.po_fwd, rec.po_bwd);
    
    float sample_weight = 1./num_pairs;
    Token const* o = (m_fwd ? m_bitext->T2 : m_bitext->T1)->sntStart(rec.sid);
    
    // adjust offsets in phrase-internal aligment
    for (size_t k = 1; k < aln.size(); k += 2) aln[k] += rec.s2 - rec.s1;
    
    vector<uint64_t> seen; seen.reserve(10);
    // It is possible that the phrase extraction extracts the same
    // phrase twice, e.g., when word a co-occurs with sequence b b b
    // but is aligned only to the middle word. We can only count
    // each phrase pair once per source phrase occurrence, or else
    // run the risk of having more joint counts than marginal
    // counts.
    
    for (size_t s = rec.s1; s <= rec.s2; ++s)
      {
        TSA<Token> const& I = m_fwd ? *m_bitext->I2 : *m_bitext->I1;
        sptr<tsa_iter> b = I.find(o + s, rec.e1 - s);
        UTIL_THROW_IF2(!b || b->size() < rec.e1 - s, "target phrase not found");
	
        for (size_t i = rec.e1; i <= rec.e2; ++i)
          {
            uint64_t tpid = b->getPid();
            
            // poor man's protection against over-counting
            size_t s = 0;
            while (s < seen.size() && seen[s] != tpid) ++s;
            if (s < seen.size()) continue;
            seen.push_back(tpid);
            
            size_t raw2 = b->approxOccurrenceCount();
            m_stats->add(tpid, sample_weight, m_bias ? (*m_bias)[p.sid] : 1, 
                         aln, raw2, rec.po_fwd, rec.po_bwd, docid);
            bool ok = (i == rec.e2) || b->extend(o[i].id());
            UTIL_THROW_IF2(!ok, "Could not extend target phrase.");
          }
        if (s < rec.s2) // shift phrase-internal alignments
          for (size_t k = 1; k < aln.size(); k += 2)
            --aln[k];
      }
  }
  
  template<typename Token>
  bool
  BitextSampler<Token>::
  operator()()
  {
    if (m_finished) return true;
    boost::unique_lock<boost::mutex> lock(m_lock);
    perform_ranked_sampling(); 
    m_finished = true;
    m_ready.notify_all();
    return true;
  }

  
  template<typename Token>
  bool
  BitextSampler<Token>::
  done() const 
  {
    return m_next == m_stop;
  }

  template<typename Token>
  sptr<pstats> 
  BitextSampler<Token>::
  stats() 
  {
    // if (m_ctr == 0) (*this)();
    // boost::unique_lock<boost::mutex> lock(m_lock);
    // while (!m_finished)
    // m_ready.wait(lock);
    return m_stats;
  }

  template<typename Token>
  BitextSampler<Token>::
  ~BitextSampler()
  { 
    m_stats->release();
  }

} // end of namespace bitext
} // end of namespace Moses

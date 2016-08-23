// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil -*-
#pragma once

#include <algorithm>

#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/math/distributions/binomial.hpp>

#include "ug_bitext.h"
#include "ug_bitext_pstats.h"
#include "ug_sampling_bias.h"
#include "ug_tsa_array_entry.h"
#include "ug_bitext_phrase_extraction_record.h"
#include "moses/TranslationModel/UG/generic/threading/ug_ref_counter.h"
#include "moses/TranslationModel/UG/generic/threading/ug_thread_safe_counter.h"
#include "moses/TranslationModel/UG/generic/sorting/NBestList.h"
namespace sapt
{
  
enum 
sampling_method 
  { 
    full_coverage, 
    random_sampling, 
    ranked_sampling, 
    ranked_sampling2 
  };
  
typedef ttrack::Position TokenPosition;
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
BitextSampler : public Moses::reference_counter
{
  typedef Bitext<Token> bitext;
  typedef TSA<Token>       tsa;
  typedef SamplingBias    bias_t;
  typedef typename Bitext<Token>::iter tsa_iter;
  mutable boost::condition_variable   m_ready; 
  mutable boost::mutex                 m_lock; 
  // const members
  // SPTR<bitext const> const   m_bitext; // keep bitext alive while I am 
  // should be an 
  SPTR<bitext const> const       m_bitext; // keep bitext alive as long as I am 
  size_t             const         m_plen; // length of lookup phrase
  bool               const          m_fwd; // forward or backward direction?
  SPTR<tsa const>    const         m_root; // root of suffix array
  char               const*        m_next; // current position
  char               const*        m_stop; // end of search range
  sampling_method    const       m_method; // look at all/random/ranked samples 
  SPTR<bias_t const> const         m_bias; // bias over candidates
  size_t             const      m_samples; // how many samples at most 
  size_t             const  m_min_samples;
  // non-const members
  SPTR<pstats>                m_stats; // destination for phrase stats
  size_t                        m_ctr; // number of samples considered
  float                  m_total_bias; // for random sampling with bias
  bool                     m_finished;
  size_t m_num_occurrences; // estimated number of phrase occurrences in corpus
  boost::taus88 m_rnd;  // every job has its own pseudo random generator
  double m_bias_total;
  bool m_track_sids; // track sentence ids in stats?

  size_t consider_sample(TokenPosition const& p);
  size_t perform_random_sampling();
  size_t perform_full_phrase_extraction();

  int check_sample_distribution(uint64_t const& sid, uint64_t const& offset);
  bool flip_coin(id_type const& sid, ushort const& offset, SamplingBias const* bias);
    
public:
  BitextSampler(BitextSampler const& other);
  // BitextSampler const& operator=(BitextSampler const& other);
  BitextSampler(SPTR<bitext const> const& bitext, 
                typename bitext::iter const& phrase,
                SPTR<SamplingBias const> const& bias, 
                size_t const min_samples, 
                size_t const max_samples,
                sampling_method const method,
                bool const track_sids);
  ~BitextSampler();
  SPTR<pstats> stats();
  bool done() const;
#ifdef MMT
#include "mmt_bitext_sampler-inc.h"
#else
  bool operator()(); // run sampling
#endif
};

template<typename Token>
int 
BitextSampler<Token>::
check_sample_distribution(uint64_t const& sid, uint64_t const& offset)
{ // ensure that the sampled distribution approximately matches the bias
  // @return 0: SKIP this occurrence
  // @return 1: consider this occurrence for sampling
  // @return 2: include this occurrence in the sample by all means

  typedef boost::math::binomial_distribution<> binomial;

  // std::ostream* log = m_bias->loglevel > 1 ? m_bias->log : NULL;
  std::ostream* log = NULL;

  if (!m_bias) return 1;

  float p = (*m_bias)[sid];
  id_type docid = m_bias->GetClass(sid);
 
  pstats::indoc_map_t::const_iterator m = m_stats->indoc.find(docid);
  uint32_t k = m != m_stats->indoc.end() ? m->second : 0 ;

  // always consider candidates from dominating documents and
  // from documents that have not been considered at all yet
  bool ret =  (p > .5 || k == 0);

  if (ret && !log) return 1;

  uint32_t N = m_stats->good; // number of trials
  float d = cdf(complement(binomial(N, p), k));
  // d: probability that samples contains k or more instances from doc #docid
  ret = ret || d >= .05;

#if 0
  if (log)
    {
      Token const* t = m_root->getCorpus()->sntStart(sid)+offset;
      Token const* x = t - min(offset,uint64_t(3));
      Token const* e = t + 4;
      if (e > m_root->getCorpus()->sntEnd(sid))
        e = m_root->getCorpus()->sntEnd(sid);
      *log << docid << ":" << sid << " " << size_t(k) << "/" << N
           << " @" << p << " => " << d << " [";
      pstats::indoc_map_t::const_iterator m;
      for (m = m_stats->indoc.begin(); m != m_stats->indoc.end(); ++m)
        {
          if (m != m_stats->indoc.begin()) *log << " ";
          *log << m->first << ":" << m->second;
        }
      *log << "] ";
      for (; x < e; ++x) *log << (*m_bitext->V1)[x->id()] << " ";
      if (!ret) *log << "SKIP";
      else if (p < .5 && d > .9) *log << "FORCE";
      *log << std::endl;
    }
#endif
  return (ret ? (p < .5 && d > .9) ? 2 : 1 : 0);
}

template<typename Token>
bool 
BitextSampler<Token>::
flip_coin(id_type const& sid, ushort const& offset, bias_t const* bias)
{
  int no_maybe_yes = bias ? check_sample_distribution(sid, offset) : 1;
  if (no_maybe_yes == 0) return false; // no
  if (no_maybe_yes > 1)  return true;  // yes
  // ... maybe: flip a coin
  size_t options_chosen = m_stats->good;
  size_t options_total  = std::max(m_stats->raw_cnt, m_ctr);
  size_t options_left   = (options_total - m_ctr);
  size_t random_number  = options_left * (m_rnd()/(m_rnd.max()+1.));
  size_t threshold;
  if (bias && m_bias_total > 0) // we have a bias and there are candidates with non-zero prob
    threshold = ((*bias)[sid]/m_bias_total * options_total * m_samples);
  else // no bias, or all have prob 0 (can happen with a very opinionated bias)
    threshold = m_samples;
  return random_number + options_chosen < threshold;
}




template<typename Token>
BitextSampler<Token>::
BitextSampler(SPTR<Bitext<Token> const> const& bitext, 
              typename bitext::iter const& phrase,
              SPTR<SamplingBias const> const& bias, size_t const min_samples, size_t const max_samples,
              sampling_method const method, bool const track_sids)
  : m_bitext(bitext)
  , m_plen(phrase.size())
  , m_fwd(phrase.root == bitext->I1.get())
  , m_root(m_fwd ? bitext->I1 : bitext->I2)
  , m_next(phrase.lower_bound(-1))
  , m_stop(phrase.upper_bound(-1))
  , m_method(method)
  , m_bias(bias)
  , m_samples(max_samples)
  , m_min_samples(min_samples)
  , m_ctr(0)
  , m_total_bias(0)
  , m_finished(false)
  , m_num_occurrences(phrase.ca())
  , m_rnd(0)
  , m_track_sids(track_sids)
{
  m_stats.reset(new pstats(m_track_sids));
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
  , m_min_samples(other.m_min_samples)
  , m_num_occurrences(other.m_num_occurrences)
  , m_rnd(0)
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

// Uniform sampling 
template<typename Token>
size_t
BitextSampler<Token>::
perform_full_phrase_extraction()
{
  if (m_next == m_stop) return m_ctr;
  for (sapt::tsa::ArrayEntry I(m_next); I.next < m_stop; ++m_ctr)
    {
      ++m_ctr;
      m_root->readEntry(I.next, I);
      consider_sample(I);
    }
  return m_ctr;
}


// Uniform sampling 
template<typename Token>
size_t
BitextSampler<Token>::
perform_random_sampling()
{
  if (m_next == m_stop) return m_ctr;
  m_bias_total = 0;
  sapt::tsa::ArrayEntry I(m_next);
  if (m_bias)
    {
      m_stats->raw_cnt = 0;
      while (I.next < m_stop)
        {
          m_root->readEntry(I.next,I);
          ++m_stats->raw_cnt;
          m_bias_total += (*m_bias)[I.sid];
        }
      I.next = m_next;
    }
      
  while (m_stats->good < m_samples && I.next < m_stop)
    {
      ++m_ctr;
      m_root->readEntry(I.next,I);
      if (!flip_coin(I.sid, I.offset, m_bias.get())) continue;
      consider_sample(I);
    }
  return m_ctr;
}

template<typename Token>
size_t
BitextSampler<Token>::
consider_sample(TokenPosition const& p)
{
  std::vector<unsigned char> aln; 
  bitvector full_aln(100*100);
  PhraseExtractionRecord 
    rec(p.sid, p.offset, p.offset + m_plen, !m_fwd, &aln, &full_aln);
  int docid = m_bias ? m_bias->GetClass(p.sid) : m_bitext->sid2did(p.sid);
  if (!m_bitext->find_trg_phr_bounds(rec))
    { // no good, probably because phrase is not coherent
      m_stats->count_sample(docid, 0, rec.po_fwd, rec.po_bwd);
      return 0;
    }
    
  // all good: register this sample as valid
  size_t num_pairs = (rec.s2 - rec.s1 + 1) * (rec.e2 - rec.e1 + 1);
  m_stats->count_sample(docid, num_pairs, rec.po_fwd, rec.po_bwd);
    
  float sample_weight = 1./num_pairs;
  Token const* o = (m_fwd ? m_bitext->T2 : m_bitext->T1)->sntStart(rec.sid);
    
  // adjust offsets in phrase-internal aligment
  for (size_t k = 1; k < aln.size(); k += 2) 
    aln[k] += rec.s2 - rec.s1;
    
  std::vector<uint64_t> seen; seen.reserve(10);
  // It is possible that the phrase extraction extracts the same
  // phrase twice, e.g., when word a co-occurs with sequence b b b but
  // is aligned only to the middle word. We can only count each phrase
  // pair once per source phrase occurrence, or else run the risk of
  // having more joint counts than marginal counts.
    
  size_t max_evidence = 0;
  for (size_t s = rec.s1; s <= rec.s2; ++s)
    {
      TSA<Token> const& I = m_fwd ? *m_bitext->I2 : *m_bitext->I1;
      SPTR<tsa_iter> b = I.find(o + s, rec.e1 - s);
      UTIL_THROW_IF2(!b || b->size() < rec.e1 - s, "target phrase not found");
	
      for (size_t i = rec.e1; i <= rec.e2; ++i)
        {
          uint64_t tpid = b->getPid();
          if (find(seen.begin(), seen.end(), tpid) != seen.end()) 
            continue; // don't over-count
          seen.push_back(tpid);
          size_t raw2 = b->approxOccurrenceCount();
          size_t evid = m_stats->add(tpid, sample_weight, 
                                     m_bias ? (*m_bias)[p.sid] : 1, 
                                     aln, raw2, rec.po_fwd, rec.po_bwd, docid,
                                     p.sid);
          max_evidence = std::max(max_evidence, evid);
          bool ok = (i == rec.e2) || b->extend(o[i].id());
          UTIL_THROW_IF2(!ok, "Could not extend target phrase.");
        }
      if (s < rec.s2) // shift phrase-internal alignments
        for (size_t k = 1; k < aln.size(); k += 2)
          --aln[k];
    }
  return max_evidence;
}
  
#ifndef MMT
template<typename Token>
bool
BitextSampler<Token>::
operator()()
{
  if (m_finished) return true;
  boost::unique_lock<boost::mutex> lock(m_lock);
  if (m_method == full_coverage)
    perform_full_phrase_extraction(); // consider all occurrences 
  else if (m_method == random_sampling)
    perform_random_sampling();
  else UTIL_THROW2("Unsupported sampling method.");
  m_finished = true;
  m_ready.notify_all();
  return true;
}
#endif

  
template<typename Token>
bool
BitextSampler<Token>::
done() const 
{
  return m_next == m_stop;
}

template<typename Token>
SPTR<pstats> 
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

} // end of namespace sapt


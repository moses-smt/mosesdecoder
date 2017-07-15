// -*- c++ -*-
// lexical phrase scorer, version 1
// written by Ulrich Germann

// Is the +1 in computing the lexical probabilities taken from the original phrase-scoring code?

#ifndef __ug_lexical_phrase_scorer_h
#define __ug_lexical_phrase_scorer_h

#include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"
#include "tpt_tokenindex.h"
#include <string>
#include <boost/unordered_map.hpp>
#include "tpt_pickler.h"
#include "ug_mm_2d_table.h"
#include "util/exception.hh"

namespace sapt
{

  template<typename TKN>
  class
  LexicalPhraseScorer2
  {
    std::vector<std::string> ftag;
  public:
    typedef mm2dTable<id_type,id_type,uint32_t,uint32_t> table_t;
    table_t COOC;
    void open(std::string const& fname);
    template<typename some_int>
    void
    score(TKN const* snt1, size_t const s1, size_t const e1,
	  TKN const* snt2, size_t const s2, size_t const e2,
	  std::vector<some_int> const & aln, float const alpha,
	  float & fwd_score, float& bwd_score) const;

    void
    score(TKN const* snt1, size_t const s1, size_t const e1,
	  TKN const* snt2, size_t const s2, size_t const e2,
	  char const* const aln_start, char const* const aln_end,
	  float const alpha, float & fwd_score, float& bwd_score) const;

    // plup: permissive lookup
    float plup_fwd(id_type const s,id_type const t, float const alpha) const;
    float plup_bwd(id_type const s,id_type const t, float const alpha) const;
    // to be done:
    // - on-the-fly smoothing ?
    // - better (than permissive-lookup) treatment of unknown combinations
    //   permissive lookup is currently used for compatibility reasons
    // - zens-ney smoothed scoring via noisy-or combination
  };

  template<typename TKN>
  void
  LexicalPhraseScorer2<TKN>::
  open(std::string const& fname)
  {
    COOC.open(fname);
  }

  template<typename TKN>
  template<typename some_int>
  void
  LexicalPhraseScorer2<TKN>::
  score(TKN const* snt1, size_t const s1, size_t const e1,
	TKN const* snt2, size_t const s2, size_t const e2,
	std::vector<some_int> const & aln, float const alpha,
	float & fwd_score, float& bwd_score) const
  {
    std::vector<float> p1(e1,0), p2(e2,0);
    std::vector<int>   c1(e1,0), c2(e2,0);
    size_t i1=0,i2=0;
    for (size_t k = 0; k < aln.size(); ++k)
      {
	i1 = aln[k]; i2 = aln[++k];
	if (i1 < s1 || i1 >= e1 || i2 < s2 || i2 >= e2) continue;
	p1[i1] += plup_fwd(snt1[i1].id(),snt2[i2].id(),alpha);
	++c1[i1];
	p2[i2] += plup_bwd(snt1[i1].id(),snt2[i2].id(),alpha);
	++c2[i2];
      }
    fwd_score = 0;
    for (size_t i = s1; i < e1; ++i)
      {
	if (c1[i] == 1) fwd_score += log(p1[i]);
	else if (c1[i]) fwd_score += log(p1[i])-log(c1[i]);
	else            fwd_score += log(plup_fwd(snt1[i].id(),0,alpha));
      }
    bwd_score = 0;
    for (size_t i = s2; i < e2; ++i)
      {
	if (c2[i] == 1) bwd_score += log(p2[i]);
	else if (c2[i]) bwd_score += log(p2[i])-log(c2[i]);
	else            bwd_score += log(plup_bwd(0,snt2[i].id(),alpha));
      }
  }

  template<typename TKN>
  float
  LexicalPhraseScorer2<TKN>::
  plup_fwd(id_type const s, id_type const t, float const alpha) const
  {
    if (COOC.m1(s) == 0 || COOC.m2(t) == 0) return 1.0;
    UTIL_THROW_IF2(alpha < 0,"At " << __FILE__ << ":" << __LINE__
		   << ": alpha parameter must be >= 0");
    float ret = COOC[s][t]+alpha;
    ret =  (ret?ret:1.)/(COOC.m1(s)+alpha);
    UTIL_THROW_IF2(ret <= 0 || ret > 1, "At " << __FILE__ << ":" << __LINE__
		   << ": result not > 0 and <= 1. alpha = " << alpha << "; "
		   << COOC[s][t] << "/" << COOC.m1(s));

#if 0
    cerr << "[" << s << "," << t << "] "
	 << COOC.m1(s) << "/"
	 << COOC[s][t] << "/"
	 << COOC.m2(t) << std::endl;
#endif
    return ret;
  }

  template<typename TKN>
  float
  LexicalPhraseScorer2<TKN>::
  plup_bwd(id_type const s, id_type const t,float const alpha) const
  {
    if (COOC.m1(s) == 0 || COOC.m2(t) == 0) return 1.0;
    UTIL_THROW_IF2(alpha < 0,"At " << __FILE__ << ":" << __LINE__
		   << ": alpha parameter must be >= 0");
    float ret = float(COOC[s][t]+alpha);
    ret = (ret?ret:1.)/(COOC.m2(t)+alpha);
    UTIL_THROW_IF2(ret <= 0 || ret > 1, "At " << __FILE__ << ":" << __LINE__
		   << ": result not > 0 and <= 1.");
    return ret;
  }

  template<typename TKN>
  void
  LexicalPhraseScorer2<TKN>::
  score(TKN const* snt1, size_t const s1, size_t const e1,
	TKN const* snt2, size_t const s2, size_t const e2,
	char const* const aln_start, char const* const aln_end,
	float const alpha, float & fwd_score, float& bwd_score) const
  {
    std::vector<float> p1(e1,0), p2(e2,0);
    std::vector<int>   c1(e1,0), c2(e2,0);
    size_t i1=0,i2=0;
    for (char const* x = aln_start; x < aln_end;)
      {
	x = tpt::binread(tpt::binread(x,i1),i2);
	if (i1 < s1 || i1 >= e1 || i2 < s2 || i2 >= e2) continue;
	p1[i1] += plup_fwd(snt1[i1].id(), snt2[i2].id(),alpha);
	++c1[i1];
	p2[i2] += plup_bwd(snt1[i1].id(), snt2[i2].id(),alpha);
	++c2[i2];
      }
    fwd_score = 0;
    for (size_t i = s1; i < e1; ++i)
      {
	if (c1[i] == 1) fwd_score += log(p1[i]);
	else if (c1[i]) fwd_score += log(p1[i])-log(c1[i]);
	else            fwd_score += log(plup_fwd(snt1[i].id(),0,alpha));
      }
    bwd_score = 0;
    for (size_t i = s2; i < e2; ++i)
      {
	if (c2[i] == 1) bwd_score += log(p2[i]);
	else if (c2[i]) bwd_score += log(p2[i])-log(c2[i]);
	else            bwd_score += log(plup_bwd(0,snt2[i].id(),alpha));
      }
  }
}
#endif

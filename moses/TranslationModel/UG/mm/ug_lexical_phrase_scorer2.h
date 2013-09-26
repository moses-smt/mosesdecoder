// -*- c++ -*-
// lexical phrase scorer, version 1
// written by Ulrich Germann

#ifndef __ug_lexical_phrase_scorer_h
#define __ug_lexical_phrase_scorer_h

#include "moses/generic/file_io/ug_stream.h"
#include "tpt_tokenindex.h"
#include <string>
#include <boost/unordered_map.hpp>
#include "tpt_pickler.h"
#include "ug_mm_2d_table.h"
using namespace std;
namespace ugdiss
{

  template<typename TKN> 
  class 
  LexicalPhraseScorer2
  {
    typedef mm2dTable<id_type,id_type,uint32_t,uint32_t> table_t;
    table_t COOC;
  public:
    void open(string const& fname);

    template<typename someint>
    void 
    score(TKN const* snt1, size_t const s1, size_t const e1,
	  TKN const* snt2, size_t const s2, size_t const e2,
	  vector<someint> & aln, float & fwd_score, float& bwd_score) const;

    void 
    score(TKN const* snt1, size_t const s1, size_t const e1,
	  TKN const* snt2, size_t const s2, size_t const e2,
	  char const* const aln_start, char const* const aln_end,
	  float & fwd_score, float& bwd_score) const;
    // plup: permissive lookup
    float plup_fwd(id_type const s,id_type const t) const; 
    float plup_bwd(id_type const s,id_type const t) const;
    // to be done: 
    // - on-the-fly smoothing ? 
    // - better (than permissive-lookup) treatment of unknown combinations 
    //   permissive lookup is currently used for compatibility reasons
    // - zens-ney smoothed scoring via noisy-or combination
  };
  
  template<typename TKN>
  void
  LexicalPhraseScorer2<TKN>::
  open(string const& fname)
  {
    COOC.open(fname);
  }

  template<typename TKN>
  template<typename someint>
  void
  LexicalPhraseScorer2<TKN>::
  score(TKN const* snt1, size_t const s1, size_t const e1,
	TKN const* snt2, size_t const s2, size_t const e2,
	vector<someint> & aln, float & fwd_score, float& bwd_score) const
  {
    vector<float> p1(e1,0), p2(e2,0);
    vector<int>   c1(e1,0), c2(e2,0);
    size_t i1=0,i2=0;
    for (size_t k = 0; k < aln.size(); ++k)
      {
	i1 = aln[k]; i2 = aln[++k];
	if (i1 < s1 || i1 >= e1 || i2 < s2 || i2 >= e2) continue;
	p1[i1] += plup_fwd(snt1[i1].id(),snt2[i2].id()); 
	++c1[i1];
	p2[i2] += plup_bwd(snt1[i1].id(),snt2[i2].id()); 
	++c2[i2];
      }
    fwd_score = 0;
    for (size_t i = s1; i < e1; ++i)
      {
	if (c1[i] == 1) fwd_score += log(p1[i]);
	else if (c1[i]) fwd_score += log(p1[i])-log(c1[i]);
	else            fwd_score += log(plup_fwd(snt1[i].id(),0));
      }
    bwd_score = 0;
    for (size_t i = s2; i < e2; ++i)
      {
	if (c2[i] == 1) bwd_score += log(p2[i]);
	else if (c2[i]) bwd_score += log(p2[i])-log(c2[i]);
	else            bwd_score += log(plup_bwd(0,snt2[i].id()));
      }
  }

  template<typename TKN>
  float
  LexicalPhraseScorer2<TKN>::
  plup_fwd(id_type const s, id_type const t) const
  {
    if (COOC.m1(s) == 0 || COOC.m2(t) == 0) return 1.0;
    // if (!COOC[s][t]) cout << s << " " << t << endl;
    assert(COOC[s][t]);
    return float(COOC[s][t])/COOC.m1(s);
  }

  template<typename TKN>
  float
  LexicalPhraseScorer2<TKN>::
  plup_bwd(id_type const s, id_type const t) const
  {
    if (COOC.m1(s) == 0 || COOC.m2(t) == 0) return 1.0;
    assert(COOC[s][t]);
    return float(COOC[s][t])/COOC.m2(t);
  }

  template<typename TKN>
  void
  LexicalPhraseScorer2<TKN>::
  score(TKN const* snt1, size_t const s1, size_t const e1,
	TKN const* snt2, size_t const s2, size_t const e2,
	char const* const aln_start, char const* const aln_end,
	float & fwd_score, float& bwd_score) const
  {
    vector<float> p1(e1,0), p2(e2,0);
    vector<int>   c1(e1,0), c2(e2,0);
    size_t i1=0,i2=0;
    for (char const* x = aln_start; x < aln_end;)
      {
	x = binread(binread(x,i1),i2);
	if (i1 < s1 || i1 >= e1 || i2 < s2 || i2 >= e2) continue;
	p1[i1] += plup_fwd(snt1[i1].id(), snt2[i2].id()); 
	++c1[i1];
	p2[i2] += plup_bwd(snt1[i1].id(), snt2[i2].id()); 
	++c2[i2];
      }
    fwd_score = 0;
    for (size_t i = s1; i < e1; ++i)
      {
	if (c1[i] == 1) fwd_score += log(p1[i]);
	else if (c1[i]) fwd_score += log(p1[i])-log(c1[i]);
	else            fwd_score += log(plup_fwd(snt1[i].id(),0));
      }
    bwd_score = 0;
    for (size_t i = s2; i < e2; ++i)
      {
	if (c2[i] == 1) bwd_score += log(p2[i]);
	else if (c2[i]) bwd_score += log(p2[i])-log(c2[i]);
	else            bwd_score += log(plup_bwd(0,snt2[i].id()));
      }
  }
}
#endif

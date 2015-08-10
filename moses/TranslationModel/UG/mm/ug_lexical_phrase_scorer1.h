// -*- c++ -*-
// lexical phrase scorer, version 1
// written by Ulrich Germann

#ifndef __ug_lexical_phrase_scorer_h
#define __ug_lexical_phrase_scorer_h

#include "ug_stream.h"
#include "tpt_tokenindex.h"
#include <string>
#include <boost/unordered_map.hpp>
#include "tpt_pickler.h"

namespace ugdiss
{

  template<typename TKN>
  class
  LexicalPhraseScorer1
  {
    typedef boost::unordered_map<id_type, float> inner_map_t;
    std::vector<inner_map_t> L1_given_L2;
    std::vector<inner_map_t> L2_given_L1;
    void load_lex (string const& fname, TokenIndex & V1, TokenIndex & V2,
		   std::vector<inner_map_t> & lex);
  public:
    void open(string const& bname, string const& L1, string const& L2,
	      TokenIndex & V1, TokenIndex & V2);
    void score(TKN const* snt1, size_t const s1, size_t const e1,
	       TKN const* snt2, size_t const s2, size_t const e2,
	       std::vector<ushort> aln, float & fwd_score, float& bwd_score);
    void score(TKN const* snt1, size_t const s1, size_t const e1,
	       TKN const* snt2, size_t const s2, size_t const e2,
	       char const* const aln_start, char const* const aln_end,
	       float & fwd_score, float& bwd_score);
    float permissive_lookup(vector<inner_map_t> const& lex,
			    id_type const s, id_type const t) const;
  };

  template<typename TKN>
  void
  LexicalPhraseScorer1<TKN>::
  load_lex (string const& fname, TokenIndex & V1, TokenIndex & V2,
	    std::vector<inner_map_t> & lex)
  {
    boost::iostreams::filtering_istream in;
    cout << fname << std::endl;
    open_input_stream(fname,in);
    lex.resize(V1.ksize());
    string w1,w2; float p;
    while (in >> w1 >> w2 >> p)
      {
	id_type id1 = V1[w1];
	while (lex.size() <= id1)
	  lex.push_back(inner_map_t());
	lex[id1][V2[w2]] = p;
      }
  }

  template<typename TKN>
  void
  LexicalPhraseScorer1<TKN>::
  open(string const& bname, string const& L1, string const& L2,
       TokenIndex & V1, TokenIndex & V2)
  {
    string lex1 = bname+L1+"-"+L2+"."+L1+"-given-"+L2+".lex.gz";
    string lex2 = bname+L1+"-"+L2+"."+L2+"-given-"+L1+".lex.gz";
    cout << lex1 << std::endl;
    cout << lex2 << std::endl;
    load_lex(lex1,V1,V2,L1_given_L2);
    load_lex(lex2,V2,V1,L2_given_L1);
  }

  template<typename TKN>
  void
  LexicalPhraseScorer1<TKN>::
  score(TKN const* snt1, size_t const s1, size_t const e1,
	TKN const* snt2, size_t const s2, size_t const e2,
	vector<ushort> aln, float & fwd_score, float& bwd_score)
  {
    std::vector<float> p1(e1,0), p2(e2,0);
    std::vector<int>   c1(e1,0), c2(e2,0);
    size_t i1=0,i2=0;
    for (size_t k = 0; k < aln.size(); ++k)
      {
	i1 = aln[k]; i2 = aln[++k];
	if (i1 < s1 || i1 >= e1 || i2 < s2 || i2 >= e2) continue;
	p1[i1] += permissive_lookup(L2_given_L1, snt2[i2].id(), snt1[i1].id());
	++c1[i1];
	p2[i2] += permissive_lookup(L1_given_L2, snt1[i1].id(), snt2[i2].id());
	++c2[i2];
      }
    fwd_score = 0;
    for (size_t i = s1; i < e1; ++i)
      {
	if (c1[i] == 1) fwd_score += log(p1[i]);
	else if (c1[i]) fwd_score += log(p1[i])-log(c1[i]);
	else            fwd_score += log(L1_given_L2[snt1[i].id()][0]);
      }
    bwd_score = 0;
    for (size_t i = s2; i < e2; ++i)
      {
	if (c2[i] == 1) bwd_score += log(p2[i]);
	else if (c2[i]) bwd_score += log(p2[i])-log(c2[i]);
	else            bwd_score += log(L2_given_L1[snt2[i].id()][0]);
      }
  }

  template<typename TKN>
  float
  LexicalPhraseScorer1<TKN>::
  permissive_lookup(vector<inner_map_t> const& lex,
		    id_type const s, id_type const t) const
  {
    if (s >= lex.size()) return 1.0;
    inner_map_t::const_iterator m = lex[s].find(t);
    return m == lex[s].end() ? 1.0 : m->second;
  }

  template<typename TKN>
  void
  LexicalPhraseScorer1<TKN>::
  score(TKN const* snt1, size_t const s1, size_t const e1,
	TKN const* snt2, size_t const s2, size_t const e2,
	char const* const aln_start, char const* const aln_end,
	float & fwd_score, float& bwd_score)
  {
    std::vector<float> p1(e1,0), p2(e2,0);
    std::vector<int>   c1(e1,0), c2(e2,0);
    size_t i1=0,i2=0;
    for (char const* x = aln_start; x < aln_end;)
      {
	x = binread(binread(x,i1),i2);
	// assert(snt1[i2].id() < L1_given_L2.size());
	// assert(snt2[i2].id() < L2_given_L1.size());
	if (i1 < s1 || i1 >= e1 || i2 < s2 || i2 >= e2) continue;
	p1[i1] += permissive_lookup(L1_given_L2, snt1[i1].id(), snt2[i2].id());
	++c1[i1];
	p2[i2] += permissive_lookup(L2_given_L1, snt2[i2].id(), snt1[i1].id());
	++c2[i2];
      }
    fwd_score = 0;
    for (size_t i = s1; i < e1; ++i)
      {
	if (c1[i] == 1) fwd_score += log(p1[i]);
	else if (c1[i]) fwd_score += log(p1[i])-log(c1[i]);
	else            fwd_score += log(L1_given_L2[snt1[i].id()][0]);
      }
    bwd_score = 0;
    for (size_t i = s2; i < e2; ++i)
      {
	if (c2[i] == 1) bwd_score += log(p2[i]);
	else if (c2[i]) bwd_score += log(p2[i])-log(c2[i]);
	else            bwd_score += log(L2_given_L1[snt2[i].id()][0]);
      }
  }
}
#endif

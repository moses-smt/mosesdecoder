// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once
#include <vector>
#include "ug_typedefs.h"
#include "ug_bitext_pstats.h"
#ifndef NO_MOSES
#include "moses/FF/LexicalReordering/LRState.h"
#endif
#include "boost/format.hpp"
#include "tpt_tokenindex.h"

namespace sapt
{

  template<typename Token>
  class
  PhrasePair
  {
  public:
    class Scorer { public: virtual float operator()(PhrasePair& pp) const = 0; };
    Token const* start1;
    Token const* start2;
    uint32_t len1;
    uint32_t len2;
    uint64_t p1, p2;
    uint32_t raw1, raw2, sample1, sample2, good1, good2, joint;
    float  cum_bias;
    std::vector<float> fvals;
    float dfwd[LRModel::NONE+1]; // distortion counts // counts or probs?
    float dbwd[LRModel::NONE+1]; // distortion counts
    std::vector<unsigned char> aln;
    float score;
    bool inverse;
    SPTR<std::vector<uint32_t> > sids; // list of sampled sentence ids where
                                       // this phrase pair was found
    // std::vector<uint32_t> indoc;
    std::map<uint32_t,uint32_t> indoc;
    PhrasePair() { };
    PhrasePair(PhrasePair const& o);

    PhrasePair const& operator+=(PhrasePair const& other);

    bool operator<(PhrasePair const& other) const;
    bool operator>(PhrasePair const& other) const;
    bool operator<=(PhrasePair const& other) const;
    bool operator>=(PhrasePair const& other) const;

    void init();
    void init(uint64_t const pid1, bool is_inverse,
	      Token const* x,   uint32_t const len,
	      pstats const* ps = NULL, size_t const numfeats=0);

    PhrasePair const&
    update(uint64_t const pid2, Token const* x,
	   uint32_t const len, jstats const& js);

    void
    fill_lr_vec(LRModel::Direction const& dir,
                LRModel::ModelType const& mdl,
                std::vector<float>& v) const;
#ifndef NO_MOSES
    void
    print(std::ostream& out, TokenIndex const& V1, TokenIndex const& V2,
          LRModel const& LR) const;
#endif 

    class SortByTargetIdSeq
    {
    public:
      int cmp(PhrasePair const& a, PhrasePair const& b) const;
      bool operator()(PhrasePair const& a, PhrasePair const& b) const;
    };

    class SortDescendingByJointCount
    {
    public:
      int cmp(PhrasePair const& a, PhrasePair const& b) const;
      bool operator()(PhrasePair const& a, PhrasePair const& b) const;
    };
  };

  template<typename Token>
  void PhrasePair<Token>
  ::init(uint64_t const pid1, bool is_inverse,
	 Token const* x, uint32_t const len,
	 pstats const* ps, size_t const numfeats)
  {
    inverse = is_inverse;
    start1 = x; len1 = len;
    p1     = pid1;
    p2     = 0;
    if (ps)
      {
        raw1    = ps->raw_cnt;
        sample1 = ps->sample_cnt;
        good1   = ps->good;
      }
    else raw1 = sample1 = good1 = 0;
    joint   = 0;
    good2   = 0;
    sample2 = 0;
    raw2    = 0;
    cum_bias = 0;
    fvals.resize(numfeats);
  }

  template<typename Token>
  PhrasePair<Token> const&
  PhrasePair<Token>
  ::update(uint64_t const pid2,
	   Token const* x, uint32_t const len, jstats const& js)
  {
    p2    = pid2;
    start2 = x; len2 = len;
    raw2  = js.cnt2();
    joint = js.rcnt();
    cum_bias = js.bcnt();
    assert(js.aln().size());
    if (js.aln().size())
      aln = js.aln()[0].second;
    // float total_fwd = 0, total_bwd = 0;
    // for (int i = 0; i <= Moses::LRModel::NONE; i++)
    // 	{
    // 	  PhraseOrientation po = static_cast<PhraseOrientation>(i);
    // 	  total_fwd += js.dcnt_fwd(po)+1;
    // 	  total_bwd += js.dcnt_bwd(po)+1;
    // 	}

    // should we do that here or leave the raw counts?
    for (int i = 0; i <= LRModel::NONE; i++)
      {
        PhraseOrientation po = static_cast<PhraseOrientation>(i);
        dfwd[i] = js.dcnt_fwd(po);
        dbwd[i] = js.dcnt_bwd(po);
      }
    
    sids = js.sids;
    indoc = js.indoc;
    return *this;
  }

  template<typename Token>
  bool
  PhrasePair<Token>
  ::operator<(PhrasePair const& other) const
  {
    return this->score < other.score;
  }

  template<typename Token>
  bool
  PhrasePair<Token>
  ::operator>(PhrasePair const& other) const
  {
    return this->score > other.score;
  }

  template<typename Token>
  bool
  PhrasePair<Token>
  ::operator<=(PhrasePair const& other) const
  {
    return this->score <= other.score;
  }

  template<typename Token>
  bool
  PhrasePair<Token>
  ::operator>=(PhrasePair const& other) const
  {
    return this->score >= other.score;
  }

  template<typename Token>
  PhrasePair<Token> const&
  PhrasePair<Token>
  ::operator+=(PhrasePair const& o)
  {
    raw1    += o.raw1;
    raw2    += o.raw2;
    good1   += o.good1;
    good2   += o.good2;
    joint   += o.joint;
    sample1 += o.sample1;
    sample2 += o.sample2;
    cum_bias += o.cum_bias;
    // todo: add distortion counts
    if (sids && o.sids)
      sids->insert(sids->end(), o.sids->begin(), o.sids->end());
    return *this;
  }

  template<typename Token>
  PhrasePair<Token>
  ::PhrasePair(PhrasePair<Token> const& o)
    : start1(o.start1)   , start2(o.start2)
    , len1(o.len1)       , len2(o.len2)
    , p1(o.p1)           , p2(o.p2)
    , raw1(o.raw1)       , raw2(o.raw2)
    , sample1(o.sample1) , sample2(o.sample2)
    ,	good1(o.good1)     , good2(o.good2)
    , joint(o.joint)     , cum_bias(o.cum_bias)  
    , fvals(o.fvals)
    , aln(o.aln)
    , score(o.score)
    , inverse(o.inverse)
    , sids(o.sids)
    , indoc(o.indoc)
  {
    for (int i = 0; i <= LRModel::NONE; ++i)
      {
	dfwd[i] = o.dfwd[i];
	dbwd[i] = o.dbwd[i];
      }
  }

  template<typename Token>
  int PhrasePair<Token>
  ::SortByTargetIdSeq
  ::cmp(PhrasePair const& a, PhrasePair const& b) const
  {
    size_t i = 0;
    Token const* x = a.start2;
    Token const* y = b.start2;
    while (i < a.len2 && i < b.len2 && x->id() == y->id())
      {
	x = x->next();
	y = y->next();
	++i;
      }
    if (i == a.len2 && i == b.len2) return 0;
    if (i == a.len2) return -1;
    if (i == b.len2) return  1;
    return x->id() < y->id() ? -1 : 1;
  }

  template<typename Token>
  bool PhrasePair<Token>
  ::SortByTargetIdSeq
  ::operator()(PhrasePair const& a, PhrasePair const& b) const
  {
    return this->cmp(a,b) < 0;
  }

  template<typename Token>
  int PhrasePair<Token>
  ::SortDescendingByJointCount
  ::cmp(PhrasePair const& a, PhrasePair const& b) const
  {
    if (a.joint == b.joint) return 0;
    return a.joint > b.joint ? -1 : 1;
  }

  template<typename Token>
  bool
  PhrasePair<Token>
  ::SortDescendingByJointCount
  ::operator()(PhrasePair const& a, PhrasePair const& b) const
  {
    return this->cmp(a,b) < 0;
  }

  template<typename Token>
  void
  PhrasePair<Token>
  ::init()
  {
    inverse = false;
    len1 = len2 = raw1 = raw2 = sample1 = sample2 = good1 = good2 = joint = 0;
    start1 = start2 = NULL;
    p1 = p2 = 0;
  }


  void
  fill_lr_vec2(LRModel::ModelType mdl, float const* const cnt,
	       float const total, float* v);

  template<typename Token>
  void
  PhrasePair<Token>
  ::fill_lr_vec(LRModel::Direction const& dir,
		LRModel::ModelType const& mdl,
		std::vector<float>& v) const
  {
    // how many distinct scores do we have?
    size_t num_scores = (mdl == LRModel::MSLR ? 4 : mdl == LRModel::MSD  ? 3 : 2);
    size_t offset;
    if (dir == LRModel::Bidirectional)
      {
        offset = num_scores;
        num_scores *= 2;
      }
    else offset = 0;

    v.resize(num_scores);

    // determine the denominator
    float total = 0;
    for (size_t i = 0; i <= LRModel::NONE; ++i)
      total += dfwd[i];

    if (dir != LRModel::Forward) // i.e., Backward or Bidirectional
      fill_lr_vec2(mdl, dbwd, total, &v[0]);
    if (dir != LRModel::Backward) // i.e., Forward or Bidirectional
      fill_lr_vec2(mdl, dfwd, total, &v[offset]);
  }


#ifndef NO_MOSES
  template<typename Token>
  void
  PhrasePair<Token>
  ::print(std::ostream& out, TokenIndex const& V1, TokenIndex const& V2,
	  LRModel const& LR) const
  {
    out << toString (V1, this->start1, this->len1) << " ::: "
	<< toString (V2, this->start2, this->len2) << " "
	<< this->joint << " [";
    // for (size_t i = 0; i < this->indoc.size(); ++i)
    for (std::map<uint32_t,uint32_t>::const_iterator m = indoc.begin();
	 m != indoc.end(); ++m)
      {
	if (m != indoc.begin()) out << " ";
	out << m->first << ":" << m->second;
      }
    out << "] [";
    std::vector<float> lrscores;
    this->fill_lr_vec(LR.GetDirection(), LR.GetModelType(), lrscores);
    for (size_t i = 0; i < lrscores.size(); ++i)
      {
	if (i) out << " ";
	out << boost::format("%.2f") % exp(lrscores[i]);
      }
    out << "]" << std::endl;
#if 0
    for (int i = 0; i <= Moses::LRModel::NONE; i++)
      {
	// PhraseOrientation po = static_cast<PhraseOrientation>(i);
	if (i) *log << " ";
	*log << p.dfwd[i];
      }
    *log << "] [";
    for (int i = 0; i <= Moses::LRModel::NONE; i++)
      {
	// PhraseOrientation po = static_cast<PhraseOrientation>(i);
	if (i) *log << " ";
	*log << p.dbwd[i];
      }
#endif
  }
#endif
} // namespace sapt

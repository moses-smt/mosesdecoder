//-*- c++ -*-
#pragma once
#include "ug_bitext.h"

using namespace ugdiss;
using namespace std;

namespace Moses {
  namespace bitext
  {

    template<typename Token>
    string 
    toString(TokenIndex const& V, Token const* x, size_t const len)
    {
      if (!len) return "";
      UTIL_THROW_IF2(!x, HERE << ": Unexpected end of phrase!");
      ostringstream buf; 
      buf << V[x->id()];
      size_t i = 1;
      for (x = x->next(); x && i < len; ++i, x = x->next())
	buf << " " << V[x->id()];
      UTIL_THROW_IF2(i != len, HERE << ": Unexpected end of phrase!");
      return buf.str();
    }

    template<typename Token>
    class 
    PhrasePair
    {
    public:
      Token const* start1;
      Token const* start2;
      uint32_t len1;
      uint32_t len2;
      // uint64_t p1, p2;
      uint32_t raw1,raw2,sample1,sample2,good1,good2,joint;
      vector<float> fvals;
      float dfwd[po_other+1]; // distortion counts // counts or probs?
      float dbwd[po_other+1]; // distortion counts
      vector<uchar> aln;
      float score;
      PhrasePair() { };
      PhrasePair(PhrasePair const& o);

      PhrasePair const& operator+=(PhrasePair const& other);

      bool operator<(PhrasePair const& other) const;
      bool operator>(PhrasePair const& other) const;
      bool operator<=(PhrasePair const& other) const; 
      bool operator>=(PhrasePair const& other) const;

      void init();
      void init(Token const* x,   uint32_t const len,
		pstats const* ps = NULL, size_t const numfeats=0);
      
      // void init(uint64_t const pid1, pstats const& ps,  size_t const numfeats);
      // void init(uint64_t const pid1, pstats const& ps1, pstats const& ps2, 
      // size_t const numfeats);

      // PhrasePair const&
      // update(uint64_t const pid2, size_t r2 = 0);

      PhrasePair const& 
      update(Token const* x, uint32_t const len, jstats const& js);
      
      // PhrasePair const& 
      // update(uint64_t const pid2, jstats   const& js1, jstats   const& js2);

      // PhrasePair const& 
      // update(uint64_t const pid2, size_t const raw2extra, jstats const& js);

      // float 
      // eval(vector<float> const& w);

      class SortByTargetIdSeq
      {
      public:
	int cmp(PhrasePair const& a, PhrasePair const& b) const;
	bool operator()(PhrasePair const& a, PhrasePair const& b) const;
      };
    };

    template<typename Token>
    void
    PhrasePair<Token>::
    init(Token const* x, uint32_t const len, 
	 pstats const* ps, size_t const numfeats)
    {
      start1 = x; len1 = len;
      // p1      = pid1;
      // p2      = 0;
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
      fvals.resize(numfeats);
    }

    template<typename Token>
    PhrasePair<Token> const&
    PhrasePair<Token>::
    update(Token const* x, uint32_t const len, jstats const& js)   
    {
      // p2    = pid2;
      start2 = x; len2 = len;
      raw2  = js.cnt2();
      joint = js.rcnt();
      assert(js.aln().size());
      if (js.aln().size()) 
	aln = js.aln()[0].second;
      float total_fwd = 0, total_bwd = 0;
      for (int i = po_first; i <= po_other; i++)
	{
	  PhraseOrientation po = static_cast<PhraseOrientation>(i);
	  total_fwd += js.dcnt_fwd(po)+1;
	  total_bwd += js.dcnt_bwd(po)+1;
	}

      // should we do that here or leave the raw counts?
      for (int i = po_first; i <= po_other; i++)
	{
	  PhraseOrientation po = static_cast<PhraseOrientation>(i);
	  dfwd[i] = float(js.dcnt_fwd(po)+1)/total_fwd;
	  dbwd[i] = float(js.dcnt_bwd(po)+1)/total_bwd;
	}

      return *this;
    }

    template<typename Token>
    bool 
    PhrasePair<Token>::
    operator<(PhrasePair const& other) const 
    { return this->score < other.score; }
    
    template<typename Token>
    bool 
    PhrasePair<Token>::
    operator>(PhrasePair const& other) const
    { return this->score > other.score; }

    template<typename Token>
    bool 
    PhrasePair<Token>::
    operator<=(PhrasePair const& other) const 
    { return this->score <= other.score; }
    
    template<typename Token>
    bool 
    PhrasePair<Token>::
    operator>=(PhrasePair const& other) const
    { return this->score >= other.score; }

    template<typename Token>
    PhrasePair<Token> const&
    PhrasePair<Token>::
    operator+=(PhrasePair const& o) 
    { 
      raw1 += o.raw1;
      raw2 += o.raw2;
      sample1 += o.sample1;
      sample2 += o.sample2;
      good1 += o.good1;
      good2 += o.good2;
      joint += o.joint;
      return *this;
    }

    template<typename Token>
    PhrasePair<Token>::
    PhrasePair(PhrasePair<Token> const& o) 
      : start1(o.start1)
      , start2(o.start2)
      , len1(o.len1)
      , len2(o.len2)
      , raw1(o.raw1) 
      , raw2(o.raw2) 
      , sample1(o.sample1)
      , sample2(o.sample2)
      ,	good1(o.good1)
      , good2(o.good2)
      , joint(o.joint)
      , fvals(o.fvals)
      , aln(o.aln)
      , score(o.score)
    {
      for (size_t i = 0; i <= po_other; ++i)
	{
	  dfwd[i] = o.dfwd[i];
	  dbwd[i] = o.dbwd[i];
	}
    }
    
    template<typename Token>
    int
    PhrasePair<Token>::
    SortByTargetIdSeq::
    cmp(PhrasePair const& a, PhrasePair const& b) const
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
    bool
    PhrasePair<Token>::
    SortByTargetIdSeq::
    operator()(PhrasePair const& a, PhrasePair const& b) const
    {
      return this->cmp(a,b) < 0;
    }

    template<typename Token>
    void 
    PhrasePair<Token>::
    init()
    {
      len1 = len2 = raw1 = raw2 = sample1 = sample2 = good1 = good2 = joint = 0;
      start1 = start2 = NULL;
    }


  } // namespace bitext
} // namespace Moses

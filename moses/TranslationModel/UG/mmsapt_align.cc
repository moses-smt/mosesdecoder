#include "mmsapt.h"

namespace Moses
{
  using namespace bitext;
  using namespace std;
  using namespace boost;
  
  struct PPgreater
  {
    bool operator()(PhrasePair const& a, PhrasePair const& b)
    {
      return a.score > b.score;
    }
  };

  void
  Mmsapt::
  setWeights(vector<float> const & w)
  {
    assert(w.size() == this->m_numScoreComponents);
    this->feature_weights = w;
  }

  struct PhraseAlnHyp
  {
    PhrasePair pp;
    ushort   s1,e1,s2,e2; // start and end positions
    int             prev; // preceding alignment hypothesis
    float          score; 
    bitvector       scov; // source coverage
    PhraseAlnHyp(PhrasePair const& ppx, int slen,
		 pair<uint32_t,uint32_t> const& sspan,
		 pair<uint32_t,uint32_t> const& tspan)
      : pp(ppx), prev(-1), score(ppx.score), scov(slen)
    {
      s1 = sspan.first; e1 = sspan.second;
      s2 = tspan.first; e2 = tspan.second;
    }

    bool operator<(PhraseAlnHyp const& other) const
    {
      return this->score < other.score;
    }

    bool operator>(PhraseAlnHyp const& other) const
    {
      return this->score > other.score;
    }

    PhraseOrientation
    po_bwd(PhraseAlnHyp const* prev) const
    {
      if (s2 == 0) return po_first;
      assert(prev);
      assert(prev->e2 <= s2);
      if (prev->e2 < s2)  return po_other;
      if (prev->e1 == s1) return po_mono;
      if (prev->e1 < s1)  return po_jfwd;
      if (prev->s1 == e1) return po_swap;
      if (prev->s1 > e1)  return po_jbwd;
      return po_other;
    }

    PhraseOrientation
    po_fwd(PhraseAlnHyp const* next) const
    {
      if (!next) return po_last;
      assert(next->s2 >= e2);
      if (next->s2 < e2)  return po_other;
      if (next->e1 == s1) return po_swap;
      if (next->e1 < s1)  return po_jbwd;
      if (next->s1 == e1) return po_mono;
      if (next->s1 > e1)  return po_jfwd;
      return po_other;
    }
  };

  sptr<vector<int> >
  Mmsapt::
  align(string const& src, string const& trg) const
  {
    // For the time being, we consult only the fixed bitext.
    // We might also consider the dynamic bitext. => TO DO.

    vector<id_type> s,t; 
    btfix.V1->fillIdSeq(src,s);
    btfix.V2->fillIdSeq(trg,t);

    vector<vector<sptr<pstats> > > 
      M1(s.size(),vector<sptr<pstats> >(s.size())),
      M2(t.size(),vector<sptr<pstats> >(t.size()));

    // get a pool of target phrase ids
    typedef vector<vector<uint64_t> > pidmap_t;
    pidmap_t sspan2pid(s.size(),vector<uint64_t>(s.size(),0));
    pidmap_t tspan2pid(t.size(),vector<uint64_t>(t.size(),0));
    typedef boost::unordered_map<uint64_t,vector<pair<uint32_t,uint32_t> > > 
      pid2span_t;
    pid2span_t spid2span,tpid2span;
    vector<vector<sptr<pstats> > > spstats(s.size());

    // Fill the lookup maps for target span to target phrase id
    // and vice versa:
    for (size_t i = 0; i < t.size(); ++i)
      {
	tsa::tree_iterator m(btfix.I2.get());
	for (size_t k = i; k < t.size() && m.extend(t[k]); ++k)
	  {
	    uint64_t pid = m.getPid();
	    tpid2span[pid].push_back(pair<uint32_t,uint32_t>(i,k+1));
	    tspan2pid[i][k] = pid;
	  }
      } // done filling lookup maps
	
    // Fill the lookup maps for the source side, and gather 
    // phrase statistics from the bitext
    for (size_t i = 0; i < s.size(); ++i)
      {
	tsa::tree_iterator m(btfix.I1.get());
	for (size_t k = i; k < s.size() && m.extend(s[k]); ++k)
	  {
	    uint64_t pid = m.getPid();
	    sspan2pid[i][k] = pid;
	    pid2span_t::iterator p = spid2span.find(pid);
	    if (p != spid2span.end())
	      {
		int x = p->second[0].first;
		int y = p->second[0].second-1;
		spstats[i].push_back(spstats[x][y-x]);
	      }
	    else spstats[i].push_back(btfix.lookup(m));
	    spid2span[pid].push_back(pair<uint32_t,uint32_t>(i,k+1));
	  }
      }
    
    // now fill the association score table
    vector<PhrasePair> PP;
    vector<PhrasePair> cands;
    vector<PhraseAlnHyp> PAH; PAH.reserve(1000000);
    vector<vector<int> > tpos2ahyp(t.size());
    typedef pid2span_t::iterator psiter;
    typedef boost::unordered_map<uint64_t,jstats> jStatsTable;
    typedef pair<uint32_t,uint32_t> xspan;
    for (psiter L = spid2span.begin(); L != spid2span.end(); ++L)
      {
	if (!L->second.size()) continue; // should never happen anyway
	int i = L->second[0].first;
	int k = L->second[0].second - i -1;
	sptr<pstats> ps = spstats[i][k];
	PhrasePair pp;
	pp.init(L->first,*ps, this->m_numScoreComponents);
	jStatsTable & J = ps->trg;
	for (jStatsTable::iterator y = J.begin(); y != J.end(); ++y)
	  {
	    psiter R = tpid2span.find(y->first);
	    if (R == tpid2span.end()) continue;
	    pp.update(y->first, y->second);
	    calc_lex(btfix,pp);
	    calc_pfwd_fix(btfix,pp);
	    calc_pbwd_fix(btfix,pp);
	    pp.eval(this->feature_weights);
	    PP.push_back(pp);
	    BOOST_FOREACH(xspan const& sspan, L->second)
	      {
		BOOST_FOREACH(xspan const& tspan, R->second)
		  {
		    tpos2ahyp[tspan.first].push_back(PAH.size());
		    PAH.push_back(PhraseAlnHyp(PP.back(),s.size(),sspan,tspan));
		  }
	      }
	  }
      }
    
    // show what we have so far ...
    for (size_t s2 = 0; s2 < t.size(); ++s2)
      {
	VectorIndexSorter<PhraseAlnHyp> foo(PAH);
	sort(tpos2ahyp[s2].begin(), tpos2ahyp[s2].end(), foo);
	for (size_t h = 0; h < tpos2ahyp[s2].size(); ++h)
	  {
	    PhraseAlnHyp const& ah = PAH[tpos2ahyp[s2][h]];
	    // pstats const & s = *ah.ps;
	    cout << setw(10) << exp(ah.score) << " "
		 << btfix.T2->pid2str(btfix.V2.get(), ah.pp.p2) 
		 << " <=> "
		 << btfix.T1->pid2str(btfix.V1.get(), ah.pp.p1);
	    vector<uchar> const& a = ah.pp.aln;
	    for (size_t u = 0; u +1 < a.size(); ++u)
	      cout << " " << int(a[u+1]) << "-" << int(a[u]);
	    cout << endl;
	    float const* ofwdj = ah.pp.dfwd;
	    float const* obwdj = ah.pp.dbwd;
	    uint32_t const* ofwdm = spstats[ah.s1][ah.e1-ah.s1-1]->ofwd;
	    uint32_t const* obwdm = spstats[ah.s1][ah.e1-ah.s1-1]->obwd;
	    cout << "   [first: " << ofwdj[po_first]<<"/"<<ofwdm[po_first]
		 <<     " last: " << ofwdj[po_last]<<"/"<<ofwdm[po_last]
		 <<     " mono: " << ofwdj[po_mono]<<"/"<<ofwdm[po_mono]
		 <<     " jfwd: " << ofwdj[po_jfwd]<<"/"<<ofwdm[po_jfwd]
		 <<     " swap: " << ofwdj[po_swap]<<"/"<<ofwdm[po_swap]
		 <<     " jbwd: " << ofwdj[po_jbwd]<<"/"<<ofwdm[po_jbwd]
		 << "]" << endl
		 << "   [first: " << obwdj[po_first]<<"/"<<obwdm[po_first]
		 <<     " last: " << obwdj[po_last]<<"/"<<obwdm[po_last]
		 <<     " mono: " << obwdj[po_mono]<<"/"<<obwdm[po_mono]
		 <<     " jfwd: " << obwdj[po_jfwd]<<"/"<<obwdm[po_jfwd]
		 <<     " swap: " << obwdj[po_swap]<<"/"<<obwdm[po_swap]
		 <<     " jbwd: " << obwdj[po_jbwd]<<"/"<<obwdm[po_jbwd]
		 << "]" << endl;
	  }
      }
    // sort(cands.begin(), cands.end(), PPgreater());
    // for (size_t i = 0; i < cands.size(); ++i)
    //   {
    // 	PhrasePair c = cands[i];
    // 	pair<uint32_t,uint32_t> & loc = spid2span[c.p1][0];
    // 	int x = loc.first;
    // 	int y = loc.second-1;
    // 	pstats& s = *spstats[x][y-x];
    // 	cout << setw(10) << exp(c.score) << " "
    // 	     << btfix.T1->pid2str(btfix.V1.get(), c.p1) << " <=> "
    // 	     << btfix.T2->pid2str(btfix.V2.get(), c.p2) << endl
    // 	     << "   [first: " << s.ofwd[po_first] 
    // 	     << " last: "  << s.ofwd[po_last] 
    // 	     << " mono: "  << s.ofwd[po_mono] 
    // 	     << " jfwd: "  << s.ofwd[po_jfwd] 
    // 	     << " swap: "  << s.ofwd[po_swap] 
    // 	     << " jbwd: "  << s.ofwd[po_jbwd] 
    // 	     << "]" << endl
    // 	     << "   [first: " << s.obwd[po_first] 
    // 	     << " last: "  << s.obwd[po_last] 
    // 	     << " mono: "  << s.obwd[po_mono] 
    // 	     << " jfwd: "  << s.obwd[po_jfwd] 
    // 	     << " swap: "  << s.obwd[po_swap] 
    // 	     << " jbwd: "  << s.obwd[po_jbwd] 
    // 	     << "]" << endl;
    //   }

    
    // boost::unordered_set<uint64_t, sptr<pstats> > smap; // target phrase ids
    // vector<vector<sptr<pstats> > > M(s.size(),vector<sptr<pstats> >(t.size());
    // for (size_t i = 0; i < s.size(); ++i)
    //   {
	
    // 	tsa::tree_iterator m(btfix.I1.get(),&s[i],&(*s.end()),false);
    // 	while (m.size())
    // 	for (size_t k = 1; k <= m.size(); ++k)
    // 	  cout << m.str(btfix.V1.get(),0,k) << " " 
    // 	       << m.approxOccurrenceCount(k-1) 
    // 	       << endl;
    //   }
    sptr<vector<int> > aln;
    return aln;
  }
}


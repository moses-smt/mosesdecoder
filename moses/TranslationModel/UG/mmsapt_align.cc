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
    assert(w.size() == this->numScoreComponents);
    this->feature_weights = w;
  }

  struct PhraseAlnHyp
  {
    int     s1,e1,s2,e2;
    uint64_t      p1,p2;
    float        pscore; 
    sptr<pstats>  ps;
    jstats const* js;
    PhraseAlnHyp(PhrasePair& pp, 
		 pair<uint32_t,uint32_t> const& sspan,
		 pair<uint32_t,uint32_t> const& tspan,
		 sptr<pstats> const& ps_,
		 jstats const* js_)
      : js(js_)
    {
      s1 = sspan.first;
      e1 = sspan.second;
      s2 = tspan.first;
      e2 = tspan.second;
      p1 = pp.p1;
      p2 = pp.p2;
      pscore = pp.score;
      ps = ps_;
    }

    PhraseAlnHyp(PhraseAlnHyp const& other)
    : s1(other.s1), e1(other.e1), s2(other.s2), e2(other.e2)
    , p1(other.p1), p2(other.p2), pscore(other.pscore), js(other.js)
    {
      ps = other.ps;
    }

    bool operator<(PhraseAlnHyp const& other) const
    {
      return this->pscore < other.pscore;
    }

    bool operator>(PhraseAlnHyp const& other) const
    {
      return this->pscore > other.pscore;
    }
  };


  sptr<vector<int> >
  Mmsapt::
  align(string const& src, string const& trg) const
  {
    // For the time being, we consult only the fixed bitext.
    // We might also consider the dynamic bitext. TO DO.

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

    for (size_t i = 0; i < t.size(); ++i)
      {
	tsa::tree_iterator m(btfix.I2.get());
	for (size_t k = i; k < t.size() && m.extend(t[k]); ++k)
	  {
	    uint64_t pid = m.getPid();
	    tpid2span[pid].push_back(pair<uint32_t,uint32_t>(i,k+1));
	    tspan2pid[i][k] = pid;
	  }
      }
	
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
    vector<vector<PhraseAlnHyp> > ahyps(t.size());
    for (pid2span_t::iterator x = spid2span.begin(); 
	 x != spid2span.end(); ++x)
      {
	int i = x->second[0].first;
	int k = x->second[0].second - i -1;
	sptr<pstats> ps = spstats[i][k];
	boost::unordered_map<uint64_t,jstats> & j = ps->trg;
	typedef boost::unordered_map<uint64_t,jstats>::iterator jiter;
	PhrasePair pp;
	pp.init(x->first,*ps, this->m_numScoreComponents);
	for (jiter y = j.begin(); y != j.end(); ++y)
	  {
	    pid2span_t::iterator z = tpid2span.find(y->first);
	    if (z != tpid2span.end())
	      {
		pp.update(y->first, y->second);
		calc_lex(btfix,pp);
		calc_pfwd_fix(btfix,pp);
		calc_pbwd_fix(btfix,pp);
		pp.eval(this->feature_weights);
		for (size_t js = 0; js < x->second.size(); ++js)
		  {
		    pair<uint32_t,uint32_t> const& sspan = x->second[js];
		    for (size_t jt = 0; jt < z->second.size(); ++jt)
		      {
			pair<uint32_t,uint32_t> & tspan = z->second[jt];
			PhraseAlnHyp pah(pp,sspan,tspan,ps,&y->second);
			ahyps[tspan.first].push_back(pah);
		      }
		  }
		// cands.push_back(pp);
		// pair<uint32_t,uint32_t> ss = x->second[0];
		// pair<uint32_t,uint32_t> ts = tpid2span[y->first][0];
		// cout << btfix.V1->toString(&s[ss.first],&s[ss.second]) 
		// << " <=> "
		// << btfix.V2->toString(&t[ts.first],&t[ts.second]) 
		// << endl;
	      }
	  }
      }
    for (size_t s2 = 0; s2 < t.size(); ++s2)
      {
	sort(ahyps[s2].begin(), ahyps[s2].end(), greater<PhraseAlnHyp>());
	for (size_t h = 0; h < ahyps[s2].size(); ++h)
	  {
	    PhraseAlnHyp const& ah = ahyps[s2][h];
	    pstats const & s = *ah.ps;
	    cout << setw(10) << exp(ah.pscore) << " "
		 << btfix.T2->pid2str(btfix.V2.get(), ah.p2) 
		 << " <=> "
		 << btfix.T1->pid2str(btfix.V1.get(), ah.p1);
	    vector<uchar> const& a = ah.js->aln()[0].second;
	    for (size_t u = 0; u +1 < a.size(); ++u)
	      cout << " " << int(a[u+1]) << "-" << int(a[u]);
	    cout << endl;
	    cout << "   [first: " << s.ofwd[po_first] 
		 << " last: "  << s.ofwd[po_last] 
		 << " mono: "  << s.ofwd[po_mono] 
		 << " jfwd: "  << s.ofwd[po_jfwd] 
		 << " swap: "  << s.ofwd[po_swap] 
		 << " jbwd: "  << s.ofwd[po_jbwd] 
		 << "]" << endl
		 << "   [first: " << s.obwd[po_first] 
		 << " last: "  << s.obwd[po_last] 
		 << " mono: "  << s.obwd[po_mono] 
		 << " jfwd: "  << s.obwd[po_jfwd] 
		 << " swap: "  << s.obwd[po_swap] 
		 << " jbwd: "  << s.obwd[po_jbwd] 
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


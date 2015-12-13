// #include "mmsapt.h"
// currently broken

// namespace Moses
// {
//   using namespace sapt;
//   using namespace std;
//   using namespace boost;

//   struct PPgreater
//   {
//     bool operator()(PhrasePair const& a, PhrasePair const& b)
//     {
//       return a.score > b.score;
//     }
//   };

//   void
//   Mmsapt::
//   setWeights(vector<float> const & w)
//   {
//     assert(w.size() == this->m_numScoreComponents);
//     this->feature_weights = w;
//   }

//   struct PhraseAlnHyp
//   {
//     PhrasePair pp;
//     ushort   s1,e1,s2,e2; // start and end positions
//     int             prev; // preceding alignment hypothesis
//     float          score;
//     bitvector       scov; // source coverage
//     PhraseAlnHyp(PhrasePair const& ppx, int slen,
// 		 pair<uint32_t,uint32_t> const& sspan,
// 		 pair<uint32_t,uint32_t> const& tspan)
//       : pp(ppx), prev(-1), score(ppx.score), scov(slen)
//     {
//       s1 = sspan.first; e1 = sspan.second;
//       s2 = tspan.first; e2 = tspan.second;
//       for (size_t i = s1; i < e1; ++i)
// 	scov.set(i);
//     }

//     bool operator<(PhraseAlnHyp const& other) const
//     {
//       return this->score < other.score;
//     }

//     bool operator>(PhraseAlnHyp const& other) const
//     {
//       return this->score > other.score;
//     }

//     PhraseOrientation
//     po_bwd(PhraseAlnHyp const* prev) const
//     {
//       if (s2 == 0) return po_first;
//       assert(prev);
//       assert(prev->e2 <= s2);
//       if (prev->e2 < s2)  return po_other;
//       if (prev->e1 == s1) return po_mono;
//       if (prev->e1 < s1)  return po_jfwd;
//       if (prev->s1 == e1) return po_swap;
//       if (prev->s1 > e1)  return po_jbwd;
//       return po_other;
//     }

//     PhraseOrientation
//     po_fwd(PhraseAlnHyp const* next) const
//     {
//       if (!next) return po_last;
//       assert(next->s2 >= e2);
//       if (next->s2 < e2)  return po_other;
//       if (next->e1 == s1) return po_swap;
//       if (next->e1 < s1)  return po_jbwd;
//       if (next->s1 == e1) return po_mono;
//       if (next->s1 > e1)  return po_jfwd;
//       return po_other;
//     }

//     float
//     dprob_fwd(PhraseAlnHyp const& next)
//     {
//       return pp.dfwd[po_fwd(&next)];
//     }

//     float
//     dprob_bwd(PhraseAlnHyp const& prev)
//     {
//       return pp.dbwd[po_bwd(&prev)];
//     }

//   };

//   class Alignment
//   {
//     typedef L2R_Token<SimpleWordId> Token;
//     typedef TSA<Token>           tsa;
//     typedef pair<uint32_t, uint32_t>  span;
//     typedef vector<vector<uint64_t> > pidmap_t; // span -> phrase ID
//     typedef boost::unordered_map<uint64_t,vector<span> > pid2span_t;
//     typedef pstats::trg_map_t jStatsTable;

//     Mmsapt const& PT;
//     vector<id_type> s,t;
//     pidmap_t   sspan2pid, tspan2pid; // span -> phrase ID
//     pid2span_t spid2span,tpid2span;
//     vector<vector<SPTR<pstats> > > spstats;

//     vector<PhrasePair> PP;
//     // position-independent phrase pair info
//   public:
//     vector<PhraseAlnHyp> PAH;
//     vector<vector<int> > tpos2ahyp;
//     // maps from target start positions to PhraseAlnHyps starting at
//     // that position

//     SPTR<pstats> getPstats(span const& sspan);
//     void fill_tspan_maps();
//     void fill_sspan_maps();
//   public:
//     Alignment(Mmsapt const& pt, string const& src, string const& trg);
//     void show(ostream& out);
//     void show(ostream& out, PhraseAlnHyp const& ah);
//   };

//   void
//   Alignment::
//   show(ostream& out, PhraseAlnHyp const& ah)
//   {
// #if 0
//     LexicalPhraseScorer2<Token>::table_t const&
//       COOCjnt = PT.calc_lex.scorer.COOC;

//     out << setw(10) << exp(ah.score) << " "
// 	<< PT.btfix.T2->pid2str(PT.btfix.V2.get(), ah.pp.p2)
// 	<< " <=> "
// 	<< PT.btfix.T1->pid2str(PT.btfix.V1.get(), ah.pp.p1);
//     vector<uchar> const& a = ah.pp.aln;
//     // BOOST_FOREACH(int x,a) cout << "[" << x << "] ";
//     for (size_t u = 0; u+1 < a.size(); u += 2)
//       out << " " << int(a[u+1]) << "-" << int(a[u]);

//     if (ah.e2-ah.s2 == 1 and ah.e1-ah.s1 == 1)
//       out << " " << COOCjnt[s[ah.s1]][t[ah.s2]]
// 	  << "/" << PT.COOCraw[s[ah.s1]][t[ah.s2]]
// 	  << "=" << float(COOCjnt[s[ah.s1]][t[ah.s2]])/PT.COOCraw[s[ah.s1]][t[ah.s2]];
//     out << endl;
//     // float const* ofwdj = ah.pp.dfwd;
//     // float const* obwdj = ah.pp.dbwd;
//     // uint32_t const* ofwdm = spstats[ah.s1][ah.e1-ah.s1-1]->ofwd;
//     // uint32_t const* obwdm = spstats[ah.s1][ah.e1-ah.s1-1]->obwd;
//     // out << "   [first: " << ofwdj[po_first]<<"/"<<ofwdm[po_first]
//     // 	 <<     " last: " << ofwdj[po_last]<<"/"<<ofwdm[po_last]
//     // 	 <<     " mono: " << ofwdj[po_mono]<<"/"<<ofwdm[po_mono]
//     // 	 <<     " jfwd: " << ofwdj[po_jfwd]<<"/"<<ofwdm[po_jfwd]
//     // 	 <<     " swap: " << ofwdj[po_swap]<<"/"<<ofwdm[po_swap]
//     // 	 <<     " jbwd: " << ofwdj[po_jbwd]<<"/"<<ofwdm[po_jbwd]
//     // 	 <<     " other: " << ofwdj[po_other]<<"/"<<ofwdm[po_other]
//     // 	 << "]" << endl
//     // 	 << "   [first: " << obwdj[po_first]<<"/"<<obwdm[po_first]
//     // 	 <<     " last: " << obwdj[po_last]<<"/"<<obwdm[po_last]
//     // 	 <<     " mono: " << obwdj[po_mono]<<"/"<<obwdm[po_mono]
//     // 	 <<     " jfwd: " << obwdj[po_jfwd]<<"/"<<obwdm[po_jfwd]
//     // 	 <<     " swap: " << obwdj[po_swap]<<"/"<<obwdm[po_swap]
//     // 	 <<     " jbwd: " << obwdj[po_jbwd]<<"/"<<obwdm[po_jbwd]
//     // 	 <<     " other: " << obwdj[po_other]<<"/"<<obwdm[po_other]
//     // 	 << "]" << endl;
// #endif
//   }

//   void
//   Alignment::
//   show(ostream& out)
//   {
//     // show what we have so far ...
//     for (size_t s2 = 0; s2 < t.size(); ++s2)
//       {
// 	VectorIndexSorter<PhraseAlnHyp> foo(PAH);
// 	sort(tpos2ahyp[s2].begin(), tpos2ahyp[s2].end(), foo);
// 	for (size_t h = 0; h < tpos2ahyp[s2].size(); ++h)
// 	  show(out,PAH[tpos2ahyp[s2][h]]);
//       }
//   }

//   SPTR<pstats>
//   Alignment::
//   getPstats(span const& sspan)
//   {
//     size_t k = sspan.second - sspan.first - 1;
//     if (k < spstats[sspan.first].size())
//       return spstats[sspan.first][k];
//     else return SPTR<pstats>();
//   }

//   void
//   Alignment::
//   fill_tspan_maps()
//   {
//     tspan2pid.assign(t.size(),vector<uint64_t>(t.size(),0));
//     for (size_t i = 0; i < t.size(); ++i)
//       {
// 	tsa::tree_iterator m(PT.btfix.I2.get());
// 	for (size_t k = i; k < t.size() && m.extend(t[k]); ++k)
// 	  {
// 	    uint64_t pid = m.getPid();
// 	    tpid2span[pid].push_back(pair<uint32_t,uint32_t>(i,k+1));
// 	    tspan2pid[i][k] = pid;
// 	  }
//       }
//   }

//   void
//   Alignment::
//   fill_sspan_maps()
//   {
//     sspan2pid.assign(s.size(),vector<uint64_t>(s.size(),0));
//     spstats.resize(s.size());
//     for (size_t i = 0; i < s.size(); ++i)
//       {
// 	tsa::tree_iterator m(PT.btfix.I1.get());
// 	for (size_t k = i; k < s.size() && m.extend(s[k]); ++k)
// 	  {
// 	    uint64_t pid = m.getPid();
// 	    sspan2pid[i][k] = pid;
// 	    pid2span_t::iterator p = spid2span.find(pid);
// 	    if (p != spid2span.end())
// 	      {
// 		int x = p->second[0].first;
// 		int y = p->second[0].second-1;
// 		spstats[i].push_back(spstats[x][y-x]);
// 	      }
// 	    else
// 	      {
// 		spstats[i].push_back(PT.btfix.lookup(m));
// 		cout << PT.btfix.T1->pid2str(PT.btfix.V1.get(),pid) << " "
// 		     << spstats[i].back()->good << "/" << spstats[i].back()->sample_cnt
// 		     << endl;
// 	      }
// 	    spid2span[pid].push_back(pair<uint32_t,uint32_t>(i,k+1));
// 	  }
//       }
//   }

//   Alignment::
//   Alignment(Mmsapt const& pt, string const& src, string const& trg)
//     : PT(pt)
//   {
//     PT.btfix.V1->fillIdSeq(src,s);
//     PT.btfix.V2->fillIdSeq(trg,t);

//     // LexicalPhraseScorer2<Token>::table_t const& COOC = PT.calc_lex.scorer.COOC;
//     // BOOST_FOREACH(id_type i, t)
//     //   {
//     // 	cout << (*PT.btfix.V2)[i];
//     // 	if (i < PT.wlex21.size())
//     // 	  {
//     // 	    BOOST_FOREACH(id_type k, PT.wlex21[i])
//     // 	      {
//     // 		size_t  j = COOC[k][i];
//     // 		size_t m1 = COOC.m1(k);
//     // 		size_t m2 = COOC.m2(i);
//     // 		if (j*1000 > m1 && j*1000 > m2)
//     // 		  cout << " " << (*PT.btfix.V1)[k];
//     // 	      }
//     // 	  }
//     // 	cout << endl;
//     //   }

//     fill_tspan_maps();
//     fill_sspan_maps();
//     tpos2ahyp.resize(t.size());
//     // now fill the association score table
//     PAH.reserve(1000000);
//     typedef pid2span_t::iterator psiter;
//     for (psiter L = spid2span.begin(); L != spid2span.end(); ++L)
//       {
// 	if (!L->second.size()) continue; // should never happen anyway
// 	int i = L->second[0].first;
// 	int k = L->second[0].second - i -1;
// 	SPTR<pstats> ps = spstats[i][k];
// 	PhrasePair pp; pp.init(L->first,*ps, PT.m_numScoreComponents);
// 	jStatsTable & J = ps->trg;
// 	for (jStatsTable::iterator y = J.begin(); y != J.end(); ++y)
// 	  {
// 	    psiter R = tpid2span.find(y->first);
// 	    if (R == tpid2span.end()) continue;
// 	    pp.update(y->first, y->second);
// 	    PT.ScorePPfix(pp);
// 	    pp.eval(PT.feature_weights);
// 	    PP.push_back(pp);
// 	    BOOST_FOREACH(span const& sspan, L->second)
// 	      {
// 		BOOST_FOREACH(span const& tspan, R->second)
// 		  {
// 		    tpos2ahyp[tspan.first].push_back(PAH.size());
// 		    PAH.push_back(PhraseAlnHyp(PP.back(),s.size(),sspan,tspan));
// 		  }
// 	      }
// 	  }
//       }
//   }



//   int
//   extend(vector<PhraseAlnHyp> & PAH, int edge, int next)
//   {
//     if ((PAH[edge].scov & PAH[next].scov).count())
//       return -1;
//     int ret = PAH.size();
//     PAH.push_back(PAH[next]);
//     PhraseAlnHyp & h = PAH.back();
//     h.prev  = edge;
//     h.scov |= PAH[edge].scov;
//     h.score += log(PAH[edge].dprob_fwd(PAH[next]));
//     h.score += log(PAH[next].dprob_bwd(PAH[edge]));
//     return ret;
//   }

//   SPTR<vector<int> >
//   Mmsapt::
//   align(string const& src, string const& trg) const
//   {
//     // For the time being, we consult only the fixed bitext.
//     // We might also consider the dynamic bitext. => TO DO.
//     Alignment A(*this,src,trg);
//     VectorIndexSorter<PhraseAlnHyp> foo(A.PAH);
//     vector<size_t> o; foo.GetOrder(o);
//     BOOST_FOREACH(int i, o) A.show(cout,A.PAH[i]);
//     SPTR<vector<int> > aln;
//     return aln;
// }
// }



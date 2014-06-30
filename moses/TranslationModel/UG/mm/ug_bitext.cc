//-*- c++ -*-

#include "ug_bitext.h"
#include <algorithm>
#include <boost/math/distributions/binomial.hpp>

using namespace ugdiss;
using namespace std;
namespace Moses
{
  namespace bitext 
  {

    ThreadSafeCounter pstats::active;
    
    pstats::
    pstats()
      : raw_cnt     (0)
      , sample_cnt  (0)
      , good        (0)
      , sum_pairs   (0)
      , in_progress (0)
    {
      ofwd[0] = ofwd[1] = ofwd[2] = ofwd[3] = ofwd[4] = ofwd[5] = ofwd[6] = 0;
      obwd[0] = obwd[1] = obwd[2] = obwd[3] = obwd[4] = obwd[5] = obwd[6] = 0;
      // if (++active%5 == 0) 
      // cerr << size_t(active) << " active pstats at " << __FILE__ << ":" << __LINE__ << endl;
    }

    pstats::
    ~pstats()
    {
      --active;
    }

    void
    pstats::
    register_worker()
    {
      this->lock.lock();
      ++this->in_progress;
      this->lock.unlock();
    }
  
    void
    pstats::
    release()
    {
      this->lock.lock();
      if (this->in_progress-- == 1) // last one - >we're done
	this->ready.notify_all();
      this->lock.unlock();
    }

    bool
    pstats::
    add(uint64_t pid, float const w, 
	vector<uchar> const& a, 
	uint32_t const cnt2, 
	uint32_t fwd_o, 
	uint32_t bwd_o)
    {
      boost::lock_guard<boost::mutex> guard(this->lock);
      jstats& entry = this->trg[pid];
      entry.add(w,a,cnt2,fwd_o,bwd_o);
      if (this->good < entry.rcnt())
	{
	  UTIL_THROW(util::Exception, "more joint counts than good counts:" 
		     << entry.rcnt() << "/" << this->good << "!");
	}
      return true;
    }

    jstats::
    jstats()
      : my_rcnt(0), my_wcnt(0), my_cnt2(0)
    { 
      ofwd[0] = ofwd[1] = ofwd[2] = ofwd[3] = ofwd[4] = ofwd[5] = ofwd[6] = 0;
      obwd[0] = obwd[1] = obwd[2] = obwd[3] = obwd[4] = obwd[5] = obwd[6] = 0;
      my_aln.reserve(1); 
    }

    jstats::
    jstats(jstats const& other)
    {
      my_rcnt = other.rcnt();
      my_wcnt = other.wcnt();
      my_aln  = other.aln();
      for (int i = po_first; i <= po_other; i++)
	{
	  ofwd[i] = other.ofwd[i];
	  obwd[i] = other.obwd[i];
	}
    }
  
    uint32_t 
    jstats::
    dcnt_fwd(PhraseOrientation const idx) const
    {
      assert(idx <= po_other);
      return ofwd[idx];
    }

    uint32_t 
    jstats::
    dcnt_bwd(PhraseOrientation const idx) const
    {
      assert(idx <= po_other);
      return obwd[idx];
    }
    
    void 
    jstats::
    add(float w, vector<uchar> const& a, uint32_t const cnt2,
	uint32_t fwd_orient, uint32_t bwd_orient)
    {
      boost::lock_guard<boost::mutex> lk(this->lock);
      my_rcnt += 1;
      my_wcnt += w;
      // my_cnt2 += cnt2; // could I really be that stupid? [UG]
      my_cnt2 = cnt2;
      if (a.size())
	{
	  size_t i = 0;
	  while (i < my_aln.size() && my_aln[i].second != a) ++i;
	  if (i == my_aln.size()) 
	    my_aln.push_back(pair<size_t,vector<uchar> >(1,a));
	  else
	    my_aln[i].first++;
	  if (my_aln[i].first > my_aln[i/2].first)
	    push_heap(my_aln.begin(),my_aln.begin()+i+1);
	}
      ++ofwd[fwd_orient];
      ++obwd[bwd_orient];
    }
    
    uint32_t 
    jstats::
    rcnt() const 
    { return my_rcnt; }
    
    float
    jstats::
    wcnt() const 
    { return my_wcnt; }

    uint32_t
    jstats::
    cnt2() const 
    { return my_cnt2; }
   
    vector<pair<size_t, vector<uchar> > > const&
    jstats::
    aln() const 
    { return my_aln; }

    void 
    jstats::
    invalidate()
    {
      my_rcnt = 0;
    }

    bool
    jstats::
    valid()
    {
      return my_rcnt != 0;
    }

    bool
    PhrasePair::
    operator<=(PhrasePair const& other) const
    {
      return this->score <= other.score;
    }

    bool
    PhrasePair::
    operator>=(PhrasePair const& other) const
    {
      return this->score >= other.score;
    }

    bool
    PhrasePair::
    operator<(PhrasePair const& other) const
    {
      return this->score < other.score;
    }
    
    bool
    PhrasePair::
    operator>(PhrasePair const& other) const
    {
      return this->score > other.score;
    }
    
    PhrasePair::
    PhrasePair() {}

    PhrasePair::
    PhrasePair(PhrasePair const& o) 
      : p1(o.p1), 
	p2(o.p2),
	raw1(o.raw1), 
	raw2(o.raw2), 
	sample1(o.sample1),
	sample2(o.sample2),
	good1(o.good1),
	good2(o.good2),
	joint(o.joint),
	fvals(o.fvals),
	aln(o.aln),
	score(o.score)
    {
      for (size_t i = 0; i <= po_other; ++i)
	{
	  dfwd[i] = o.dfwd[i];
	  dbwd[i] = o.dbwd[i];
	}
    }
    
    void
    PhrasePair::
    init(uint64_t const pid1, pstats const& ps, size_t const numfeats)
    {
      p1      = pid1;
      p2      = 0;
      raw1    = ps.raw_cnt;
      sample1 = ps.sample_cnt;
      sample2 = 0;
      good1   = ps.good;
      good2   = 0;
      raw2    = 0;
      fvals.resize(numfeats);
    }

    void
    PhrasePair::
    init(uint64_t const pid1, 
	 pstats const& ps1, 
	 pstats const& ps2, 
	 size_t const numfeats)
    {
      p1      = pid1;
      raw1    = ps1.raw_cnt    + ps2.raw_cnt;
      sample1 = ps1.sample_cnt + ps2.sample_cnt;
      sample2 = 0;
      good1   = ps1.good       + ps2.good;
      good2   = 0;
      fvals.resize(numfeats);
    }
    
    float 
    lbop(size_t const tries, size_t const succ, float const confidence)
    {
      return (confidence == 0 
	      ? float(succ)/tries 
	      : (boost::math::binomial_distribution<>::
		 find_lower_bound_on_p(tries, succ, confidence)));
    }
    
    PhrasePair const&
    PhrasePair::
    update(uint64_t const pid2, jstats const& js)   
    {
      p2    = pid2;
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
      for (int i = po_first; i <= po_other; i++)
	{
	  PhraseOrientation po = static_cast<PhraseOrientation>(i);
	  dfwd[i] = float(js.dcnt_fwd(po)+1)/total_fwd;
	  dbwd[i] = float(js.dcnt_bwd(po)+1)/total_bwd;
	}
      return *this;
    }

    PhrasePair const&
    PhrasePair::
    update(uint64_t const pid2, jstats const& js1, jstats const& js2)   
    {
      p2    = pid2;
      raw2  = js1.cnt2() + js2.cnt2();
      joint = js1.rcnt() + js2.rcnt();
      assert(js1.aln().size() || js2.aln().size());
      if (js1.aln().size()) 
	aln = js1.aln()[0].second;
      else if (js2.aln().size()) 
	aln = js2.aln()[0].second;
      for (int i = po_first; i < po_other; i++)
	{
	  PhraseOrientation po = static_cast<PhraseOrientation>(i);
	  dfwd[i] = float(js1.dcnt_fwd(po) + js2.dcnt_fwd(po) + 1)/(sample1+po_other);
	  dbwd[i] = float(js1.dcnt_bwd(po) + js2.dcnt_bwd(po) + 1)/(sample1+po_other);
	}
      return *this;
    }

    PhrasePair const&
    PhrasePair::
    update(uint64_t const pid2, 
	   size_t   const raw2extra,
	   jstats   const& js)   
    {
      p2    = pid2;
      raw2  = js.cnt2() + raw2extra;
      joint = js.rcnt();
      assert(js.aln().size());
      if (js.aln().size()) 
	aln = js.aln()[0].second;
      for (int i = po_first; i <= po_other; i++)
	{
	  PhraseOrientation po = static_cast<PhraseOrientation>(i);
	  dfwd[i] = float(js.dcnt_fwd(po)+1)/(sample1+po_other);
	  dbwd[i] = float(js.dcnt_bwd(po)+1)/(sample1+po_other);
	}
      return *this;
    }

    float
    PhrasePair::
    eval(vector<float> const& w)
    {
      assert(w.size() == this->fvals.size());
      this->score = 0;
      for (size_t i = 0; i < w.size(); ++i)
	this->score += w[i] * this->fvals[i];
      return this->score;
    }
  
    template<>
    sptr<imBitext<L2R_Token<SimpleWordId> > > 
    imBitext<L2R_Token<SimpleWordId> >::
    add(vector<string> const& s1, 
	vector<string> const& s2, 
	vector<string> const& aln) const
    {
      typedef L2R_Token<SimpleWordId> TKN;
      assert(s1.size() == s2.size() && s1.size() == aln.size());
      
#ifndef NDEBUG
      size_t first_new_snt = this->T1 ? this->T1->size() : 0;
#endif

      sptr<imBitext<TKN> > ret;
      {
	lock_guard<mutex> guard(this->lock);
	ret.reset(new imBitext<TKN>(*this));
      }
      
      // we add the sentences in separate threads (so it's faster)
      boost::thread thread1(snt_adder<TKN>(s1,*ret->V1,ret->myT1,ret->myI1));
      // thread1.join(); // for debugging
      boost::thread thread2(snt_adder<TKN>(s2,*ret->V2,ret->myT2,ret->myI2));
      BOOST_FOREACH(string const& a, aln)
	{
	  istringstream ibuf(a);
	  ostringstream obuf;
	  uint32_t row,col; char c;
	  while (ibuf >> row >> c >> col)
	    {
	      assert(c == '-');
	      binwrite(obuf,row);
	      binwrite(obuf,col);
	    }
	  // important: DO NOT replace the two lines below this comment by 
	  // char const* x = obuf.str().c_str(), as the memory x is pointing 
	  // to is freed immediately upon deconstruction of the string object.
	  string foo = obuf.str(); 
	  char const* x = foo.c_str();
	  vector<char> v(x,x+foo.size());
	  ret->myTx = append(ret->myTx, v);
	}

      thread1.join();
      thread2.join();

      ret->Tx = ret->myTx;
      ret->T1 = ret->myT1;
      ret->T2 = ret->myT2;
      ret->I1 = ret->myI1;
      ret->I2 = ret->myI2;

#ifndef NDEBUG
      // sanity check
      for (size_t i = first_new_snt; i < ret->T1->size(); ++i)
	{
	  size_t slen1  = ret->T1->sntLen(i);
	  size_t slen2  = ret->T2->sntLen(i);
	  char const* p = ret->Tx->sntStart(i);
	  char const* q = ret->Tx->sntEnd(i);
	  size_t k;
	  while (p < q)
	    {
	      p = binread(p,k);
	      assert(p);
	      assert(p < q);
	      assert(k < slen1);
	      p = binread(p,k);
	      assert(p);
	      assert(k < slen2);
	    }
	}
#endif
      return ret;
    }

    // template<>
    void
    snt_adder<L2R_Token<SimpleWordId> >::
    operator()()
    {
	vector<id_type> sids;
	sids.reserve(snt.size());
	BOOST_FOREACH(string const& foo, snt)
	  {
	    sids.push_back(track ? track->size() : 0);
	    istringstream buf(foo);
	    string w;
	    vector<L2R_Token<SimpleWordId > > s;
	    s.reserve(100);
	    while (buf >> w) 
	      s.push_back(L2R_Token<SimpleWordId>(V[w]));
	    track = append(track,s);
	  }
	if (index)
	  index.reset(new imTSA<L2R_Token<SimpleWordId> >(*index,track,sids,V.tsize()));
	else
	  index.reset(new imTSA<L2R_Token<SimpleWordId> >(track,NULL,NULL));
    }

    snt_adder<L2R_Token<SimpleWordId> >::
    snt_adder(vector<string> const& s, TokenIndex& v, 
     	      sptr<imTtrack<L2R_Token<SimpleWordId> > >& t, 
	      sptr<imTSA<L2R_Token<SimpleWordId> > >& i)
      : snt(s), V(v), track(t), index(i) 
    { }


    bool 
    expand_phrase_pair
    (vector<vector<ushort> >& a1, 
     vector<vector<ushort> >& a2,
     ushort const s2, // next word on in target side
     ushort const L1, ushort const R1, // limits of previous phrase
     ushort & s1, ushort & e1, ushort& e2) // start/end src; end trg
    {
      if (a2[s2].size() == 0) 
	{
	  cout << __FILE__ << ":" << __LINE__ << endl;
	  return false;
	}
      bitvector done1(a1.size());
      bitvector done2(a2.size());
      vector <pair<ushort,ushort> > agenda; 
      // x.first:  side (1 or 2)
      // x.second: word position
      agenda.reserve(a1.size() + a2.size());
      agenda.push_back(pair<ushort,ushort>(2,s2));
      e2 = s2;
      s1 = e1 = a2[s2].front();
      if (s1 >= L1 && s1 < R1) 
	{
	  cout << __FILE__ << ":" << __LINE__ << endl;
	  return false;
	}
      agenda.push_back(pair<ushort,ushort>(2,s2));
      while (agenda.size())
	{
	  ushort side = agenda.back().first;
	  ushort p    = agenda.back().second;
	  agenda.pop_back();
	  if (side == 1)
	    {
	      done1.set(p);
	      BOOST_FOREACH(ushort i, a1[p])
		{
		  if (i < s2) 
		    {
		      // cout << __FILE__ << ":" << __LINE__ << endl;
		      return false;
		    }
		  if (done2[i]) continue;
		  for (;e2 <= i;++e2)
		    if (!done2[e2]) 
		      agenda.push_back(pair<ushort,ushort>(2,e2));
		}
	    }
	  else
	    {
	      done2.set(p);
	      BOOST_FOREACH(ushort i, a2[p])
		{
		  if ((e1 < L1 && i >= L1) || (s1 >= R1 && i < R1) || (i >= L1 && i < R1))
		    {
		      // cout << __FILE__ << ":" << __LINE__ << " " 
		      // << L1 << "-" << R1 << " " << i << " " 
		      // << s1 << "-" << e1<< endl;
		      return false;
		    }
		  
		  if (e1 < i)
		    {
		      for (; e1 <= i; ++e1)
			if (!done1[e1])
			  agenda.push_back(pair<ushort,ushort>(1,e1));
		    }
		  else if (s1 > i)
		    {
		      for (; i <= s1; ++i)
			if (!done1[i])
			  agenda.push_back(pair<ushort,ushort>(1,i));
		    }
		}
	    }
	}
      ++e1;
      ++e2;
      return true;
    }
    //   s1 = seed;
    //   e1 = seed;
    //   s2 = e2 = a1[seed].front();

    //   BOOST_FOREACH(ushort k, a1[seed])
    // 	{
    // 	  if (s2 < k) s2 = k;
    // 	  if (e2 > k) e2 = k;
    // 	}

    //   for (ushort j = s2; j <= e2; ++j)
    // 	{
    // 	  if (a2[j].size() == 0) continue;
    // 	  done2.set(j);
    // 	  agenda.push_back(pair<ushort,ushort>(j,1));
    // 	}

    //   while (agenda.size())
    // 	{
    // 	  ushort side = agenda[0].second;
    // 	  ushort i    = agenda[0].first;
    // 	  agenda.pop_back();
    // 	  if (side)
    // 	    {
    // 	      BOOST_FOREACH(ushort k, a2[i])
    // 		{
    // 		  if (k < L1 || k > R1) 
    // 		    return false;
    // 		  if (done1[k]) 
    // 		    continue;
    // 		  while (s1 > k)
    // 		    {
    // 		      --s1;
    // 		      if (done1[s1] || !a1[s1].size()) 
    // 			continue;
    // 		      done1.set(s1);
    // 		      agenda.push_back(pair<ushort,ushort>(s1,0));
    // 		    }
    // 		  while (e1 < k)
    // 		    {
    // 		      ++e1;
    // 		      if (done1[e1] || !a1[e1].size())
    // 			continue;
    // 		      done1.set(e1);
    // 		      agenda.push_back(pair<ushort,ushort>(e1,0));
    // 		    }
    // 		}
    // 	    }
    // 	  else
    // 	    {
    // 	      BOOST_FOREACH(ushort k, a1[i])
    // 		{
    // 		  if (k < L2 || k > R2) 
    // 		    return false;
    // 		  if (done2[k]) 
    // 		    continue;
    // 		  while (s2 > k)
    // 		    {
    // 		      --s2;
    // 		      if (done2[s2] || !a2[s2].size()) 
    // 			continue;
    // 		      done1.set(s2);
    // 		      agenda.push_back(pair<ushort,ushort>(s2,1));
    // 		    }
    // 		  while (e2 < k)
    // 		    {
    // 		      ++e2;
    // 		      if (done1[e2] || !a1[e2].size())
    // 			continue;
    // 		      done2.set(e2);
    // 		      agenda.push_back(pair<ushort,ushort>(e2,1));
    // 		    }
    // 		}
    // 	    }
    // 	}
    //   ++e1;
    //   ++e2;
    //   return true;
    // }

    void 
    print_amatrix(vector<vector<ushort> > a1, uint32_t len2,
		  ushort b1, ushort e1, ushort b2, ushort e2)
    {
      vector<bitvector> M(a1.size(),bitvector(len2));
      for (ushort j = 0; j < a1.size(); ++j)
	{
	  BOOST_FOREACH(ushort k, a1[j])
	    M[j].set(k);
	}
      cout << b1 << "-" << e1 << " " << b2 << "-" << e2 << endl;
      cout << "   ";
      for (size_t c = 0; c < len2;++c)
	cout << c%10;
      cout << endl;
      for (size_t r = 0; r < M.size(); ++r)
	{
	  cout << setw(3) << r << " ";
	  for (size_t c = 0; c < M[r].size(); ++c)
	    {
	      if ((b1 <= r) && (r < e1) && b2 <= c && c < e2)
		cout << (M[r][c] ? 'x' : '-');
	      else cout << (M[r][c] ? 'o' : '.');
	    }
	  cout << endl;
	}
      cout  << string(90,'-') << endl;
    }


    PhraseOrientation 
    find_po_fwd(vector<vector<ushort> >& a1,
		vector<vector<ushort> >& a2,
		size_t b1, size_t e1,
		size_t b2, size_t e2)
    {
      size_t n2 = e2;
      while (n2 < a2.size() && a2[n2].size() == 0) ++n2;

      if (n2 == a2.size()) 
	return po_last;
      
      ushort ns1,ne1,ne2;
      if (!expand_phrase_pair(a1,a2,n2,b1,e1,ns1,ne1,ne2))
	{
	  return po_other;
	}
      if (ns1 >= e1)
	{
	  for (ushort j = e1; j < ns1; ++j)
	    if (a1[j].size()) return po_jfwd;
	  return po_mono;
	}
      else
	{
	  for (ushort j = ne1; j < b1; ++j)
	    if (a1[j].size()) return po_jbwd;
	  return po_swap;
	}
    }


    PhraseOrientation 
    find_po_bwd(vector<vector<ushort> >& a1,
		vector<vector<ushort> >& a2,
		size_t b1, size_t e1,
		size_t b2, size_t e2)
    {
      int p2 = b2-1;
      while (p2 >= 0 && !a2[p2].size()) --p2;
      if (p2 < 0) return po_first;
      ushort ps1,pe1,pe2;
      if (!expand_phrase_pair(a1,a2,p2,b1,e1,ps1,pe1,pe2))
	return po_other;
      
      if (pe1 < b1)
	{
	  for (ushort j = pe1; j < b1; ++j)
	    if (a1[j].size()) return po_jfwd;
	  return po_mono;
	}
      else
	{
	  for (ushort j = e1; j < ps1; ++j)
	    if (a1[j].size()) return po_jbwd;
	  return po_swap;
	}
    }
  }
}

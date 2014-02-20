//-*- c++-mode -*-

#include "ug_bitext.h"
#include <algorithm>
#include <boost/math/distributions/binomial.hpp>

using namespace ugdiss;
using namespace std;
namespace Moses
{
  namespace bitext 
  {
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
	uint32_t const cnt2)
    {
      this->lock.lock();
      jstats& entry = this->trg[pid];
      this->lock.unlock();
      entry.add(w,a,cnt2);
      if (this->good < entry.rcnt())
	{
	  this->lock.lock();
	  return false;
	  // UTIL_THROW(util::Exception, "more joint counts than good counts!" 
	  // 	     << entry.rcnt() << "/" << this->good);
	}
      return true;
    }

    jstats::
    jstats()
      : my_rcnt(0), my_wcnt(0), my_cnt2(0)
    { 
      my_aln.reserve(1); 
    }

    jstats::
    jstats(jstats const& other)
    {
      my_rcnt = other.rcnt();
      my_wcnt = other.wcnt();
      my_aln  = other.aln();
    }
  
    void 
    jstats::
    add(float w, vector<uchar> const& a, uint32_t const cnt2)
    {
      boost::lock_guard<boost::mutex> lk(this->lock);
      my_rcnt += 1;
      my_wcnt += w;
      my_cnt2 += cnt2;
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
    
    PhrasePair::PhrasePair() {}
    
    void
    PhrasePair::
    init(uint64_t const pid1, pstats const& ps, size_t const numfeats)
    {
      p1      = pid1;
      raw1    = ps.raw_cnt;
      sample1 = ps.sample_cnt;
      sample2 = 0;
      good1   = ps.good;
      good2   = 0;
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
      return 
	boost::math::binomial_distribution<>::
	find_lower_bound_on_p(tries, succ, confidence);
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
      
      sptr<imBitext<TKN> > ret;
      {
	lock_guard<mutex> guard(this->lock);
	ret.reset(new imBitext<TKN>(*this));
      }
      
      // we add the sentences in separate threads (so it's faster)
      boost::thread thread1(snt_adder<TKN>(s1,*ret->V1,ret->myT1,ret->myI1));
      thread1.join(); // for debugging
      boost::thread thread2(snt_adder<TKN>(s2,*ret->V2,ret->myT2,ret->myI2));
      BOOST_FOREACH(string const& a, aln)
	{
	  istringstream ibuf(a);
	  ostringstream obuf;
	  uint32_t row,col; char c;
	  while (ibuf>>row>>c>>col)
	    {
	      assert(c == '-');
	      binwrite(obuf,row);
	      binwrite(obuf,col);
	    }
	  char const* x = obuf.str().c_str();
	  vector<char> v(x,x+obuf.str().size());
	  ret->myTx = append(ret->myTx, v);
	}
      thread1.join();
      thread2.join();
      ret->Tx = ret->myTx;
      ret->T1 = ret->myT1;
      ret->T2 = ret->myT2;
      ret->I1 = ret->myI1;
      ret->I2 = ret->myI2;
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
     ushort const seed, 
     ushort const L1, // hard left  limit source
     ushort const R1, // hard right limit source
     ushort const L2, // hard left  limit target
     ushort const R2, // hard right limit target
     ushort & s1, ushort & e1, // start/end src phrase
     ushort & s2, ushort & e2) // start/end trg phrase
    {
      if (a1[seed].size() == 0) return false;
      assert(L1 <= seed);
      assert(R1 >  seed);
      bitvector done1(a1.size());
      bitvector done2(a2.size());
      vector <pair<ushort,ushort> > agenda; 
      agenda.reserve(a1.size() + a2.size());
      agenda.push_back(pair<ushort,ushort>(seed,0));
      s1 = seed;
      e1 = seed;
      s2 = e2 = a1[seed].front();

      BOOST_FOREACH(ushort k, a1[seed])
	{
	  if (s2 < k) s2 = k;
	  if (e2 > k) e2 = k;
	}

      for (ushort j = s2; j <= e2; ++j)
	{
	  if (a2[j].size() == 0) continue;
	  done2.set(j);
	  agenda.push_back(pair<ushort,ushort>(j,1));
	}

      while (agenda.size())
	{
	  ushort side = agenda[0].second;
	  ushort i    = agenda[0].first;
	  agenda.pop_back();
	  if (side)
	    {
	      BOOST_FOREACH(ushort k, a2[i])
		{
		  if (k < L1 || k > R1) 
		    return false;
		  if (done1[k]) 
		    continue;
		  while (s1 > k)
		    {
		      --s1;
		      if (done1[s1] || !a1[s1].size()) 
			continue;
		      done1.set(s1);
		      agenda.push_back(pair<ushort,ushort>(s1,0));
		    }
		  while (e1 < k)
		    {
		      ++e1;
		      if (done1[e1] || !a1[e1].size())
			continue;
		      done1.set(e1);
		      agenda.push_back(pair<ushort,ushort>(e1,0));
		    }
		}
	    }
	  else
	    {
	      BOOST_FOREACH(ushort k, a1[i])
		{
		  if (k < L2 || k > R2) 
		    return false;
		  if (done2[k]) 
		    continue;
		  while (s2 > k)
		    {
		      --s2;
		      if (done2[s2] || !a2[s2].size()) 
			continue;
		      done1.set(s2);
		      agenda.push_back(pair<ushort,ushort>(s2,1));
		    }
		  while (e2 < k)
		    {
		      ++e2;
		      if (done1[e2] || !a1[e2].size())
			continue;
		      done2.set(e2);
		      agenda.push_back(pair<ushort,ushort>(e2,1));
		    }
		}
	    }
	}
      ++e1;
      ++e2;
      return true;
    }

    PhraseOrientation 
    find_po_fwd(vector<vector<ushort> >& a1,
		vector<vector<ushort> >& a2,
		size_t b1, size_t e1,
		size_t b2, size_t e2)
    {
      size_t n2 = e2;
      while (n2 < a2.size() && a2[n2].size() == 0) ++n2;
      if (n2 == a2.size()) return po_last;
      
      ushort ns1,ns2,ne1,ne2;
      bool OK = expand_phrase_pair(a2,a1,n2,
				   e2, a2.size()-1,
				   0,  a1.size()-1,
				   ns2,ne2,ns1,ne1);
      if (!OK) return po_other;
      if (ns1 >= e1)
	{
	  for (ushort j = e1; j < ns2; ++j)
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
      ushort ps1,ps2,pe1,pe2;
      bool OK = expand_phrase_pair(a2,a1,p2,
				   0, b2-1,
				   0, a1.size()-1,
				   ps2,pe2,ps1,pe1);
      if (!OK) return po_other;
      
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
      return po_other; 
    }
  }
}

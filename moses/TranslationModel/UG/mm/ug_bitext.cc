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
    {}

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
	BOOST_FOREACH(string const& s, snt)
	  {
	    sids.push_back(track ? track->size() : 0);
	    istringstream buf(s);
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

  }
}

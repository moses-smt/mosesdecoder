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

    void
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
	  UTIL_THROW(util::Exception, "more joint counts than good counts!" 
		     << entry.rcnt() << "/" << this->good);
	}
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
    
    float 
    lbop(size_t const tries, size_t const succ, float const confidence)
    {
      return 
	boost::math::binomial_distribution<>::
	find_lower_bound_on_p(tries, succ, confidence);
    }
    
    void 
    PhrasePair::
    update(uint64_t const pid2, jstats const& js)   
    {
      p2    = pid2;
      raw2  = js.cnt2();
      joint = js.rcnt();
      assert(js.aln().size());
      if (js.aln().size()) 
	aln = js.aln()[0].second;
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
  
  }
}

#include "ug_bitext_jstats.h"
namespace Moses
{
  namespace bitext
  {

    uint32_t jstats::rcnt() const { return my_rcnt; }
    float    jstats::wcnt() const { return my_wcnt; }
    uint32_t jstats::cnt2() const { return my_cnt2; }

    // What was that used for again? UG
    bool jstats::valid() { return my_wcnt >= 0; }
    void jstats::validate()   { if (my_wcnt < 0) my_wcnt *= -1; }
    void jstats::invalidate() { if (my_wcnt > 0) my_wcnt *= -1; }

    jstats::
    jstats()
      : my_rcnt(0), my_cnt2(0), my_wcnt(0)
    { 
      for (int i = 0; i <= Moses::LRModel::NONE; ++i) 
	ofwd[i] = obwd[i] = 0;
      my_aln.reserve(1);
    }
    
    jstats::
    jstats(jstats const& other)
    {
      my_rcnt = other.rcnt();
      my_wcnt = other.wcnt();
      my_aln  = other.aln();
      indoc   = other.indoc;
      for (int i = 0; i <= Moses::LRModel::NONE; i++)
	{
	  ofwd[i] = other.ofwd[i];
	  obwd[i] = other.obwd[i];
	}
    }
  
    uint32_t 
    jstats::
    dcnt_fwd(PhraseOrientation const idx) const
    {
      assert(idx <= Moses::LRModel::NONE);
      return ofwd[idx];
    }

    uint32_t 
    jstats::
    dcnt_bwd(PhraseOrientation const idx) const
    {
      assert(idx <= Moses::LRModel::NONE);
      return obwd[idx];
    }
    
    void 
    jstats::
    add(float w, vector<uchar> const& a, uint32_t const cnt2,
	uint32_t fwd_orient, uint32_t bwd_orient, int const docid)
    {
      boost::lock_guard<boost::mutex> lk(this->lock);
      my_cnt2 = cnt2;
      my_rcnt += 1;
      my_wcnt += w;
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
      if (docid >= 0)
	{
	  while (int(indoc.size()) <= docid) indoc.push_back(0);
	  ++indoc[docid];
	}
    }

    vector<pair<size_t, vector<uchar> > > const&
    jstats::
    aln() const 
    { return my_aln; }

  } // namespace bitext
} // namespace Moses

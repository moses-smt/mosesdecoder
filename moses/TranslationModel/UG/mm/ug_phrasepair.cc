#include "ug_phrasepair.h"
namespace Moses {
  namespace bitext
  {

#if 0
    void 
    PhrasePair::
    init()
    {
      p1 = p2 = raw1 = raw2 = sample1 = sample2 = good1 = good2 = joint = 0;
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
      joint   = 0;
      fvals.resize(numfeats);
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
    update(uint64_t const pid2, size_t r2)
    {
      p2    = pid2;
      raw2  = r2;
      joint = 0;
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
#endif
  } // namespace bitext
} // namespace Moses


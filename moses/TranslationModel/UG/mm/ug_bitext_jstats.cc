// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "ug_bitext_jstats.h"
namespace sapt
{

  uint32_t jstats::rcnt() const { return my_rcnt; }
  float    jstats::wcnt() const { return my_wcnt; }
  float    jstats::bcnt() const { return my_bcnt; }
  uint32_t jstats::cnt2() const { return my_cnt2; }

  // What was that used for again? UG
  bool jstats::valid() { return my_wcnt >= 0; }
  void jstats::validate()   { if (my_wcnt < 0) my_wcnt *= -1; }
  void jstats::invalidate() { if (my_wcnt > 0) my_wcnt *= -1; }

  jstats::
  jstats()
    : my_rcnt(0), my_cnt2(0), my_wcnt(0), my_bcnt(0)
  {
    for (int i = 0; i <= LRModel::NONE; ++i)
      ofwd[i] = obwd[i] = 0;
    my_aln.reserve(1);
  }

  jstats::
  jstats(jstats const& other)
  {
    my_rcnt = other.rcnt();
    my_wcnt = other.wcnt();
    my_bcnt = other.bcnt();
    my_aln  = other.aln();
    sids = other.sids;
    indoc   = other.indoc;
    for (int i = 0; i <= LRModel::NONE; i++)
      {
        ofwd[i] = other.ofwd[i];
        obwd[i] = other.obwd[i];
      }
  }

  uint32_t
  jstats::
  dcnt_fwd(PhraseOrientation const idx) const
  {
    assert(idx <= LRModel::NONE);
    return ofwd[idx];
  }

  uint32_t
  jstats::
  dcnt_bwd(PhraseOrientation const idx) const
  {
    assert(idx <= LRModel::NONE);
    return obwd[idx];
  }

  size_t
  jstats::
  add(float w, float b, std::vector<unsigned char> const& a, uint32_t const cnt2,
      uint32_t fwd_orient, uint32_t bwd_orient, int const docid,
      uint32_t const sid, bool const track_sid)
  {
    boost::lock_guard<boost::mutex> lk(this->lock);
    my_cnt2 = cnt2;
    my_rcnt += 1;
    my_wcnt += w;
    my_bcnt += b;
    if (a.size())
      {
        size_t i = 0;
        while (i < my_aln.size() && my_aln[i].second != a) ++i;
        if (i == my_aln.size())
          my_aln.push_back(std::pair<size_t,std::vector<unsigned char> >(1,a));
        else
          my_aln[i].first++;
        if (my_aln[i].first > my_aln[i/2].first)
          push_heap(my_aln.begin(),my_aln.begin()+i+1);
      }
    ++ofwd[fwd_orient];
    ++obwd[bwd_orient];
    // Record sentence id if requested
    if (track_sid)
      {
        if (!sids)
          sids.reset(new std::vector<uint32_t>);
        sids->push_back(sid);
      }
    if (docid >= 0)
      {
        // while (int(indoc.size()) <= docid) indoc.push_back(0);
        ++indoc[docid];
      }
    return my_rcnt;
  }
  
  std::vector<std::pair<size_t, std::vector<unsigned char> > > const&
  jstats::
  aln() const
  { return my_aln; }

} // namespace sapt

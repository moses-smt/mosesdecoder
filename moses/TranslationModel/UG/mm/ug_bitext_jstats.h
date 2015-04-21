// -*- c++ -*-
#pragma once
#include "ug_typedefs.h"
#include "ug_lexical_reordering.h"
#include <boost/thread.hpp>

namespace Moses 
{
  namespace bitext
  {
    using namespace ugdiss;

    // "joint" (i.e., phrase pair) statistics    
    class
    jstats
    {
      boost::mutex lock;
      uint32_t my_rcnt; // unweighted joint count
      uint32_t my_cnt2; // raw counts L2
      float    my_wcnt; // weighted joint count 

      // to do: use a static alignment pattern store that stores each pattern only
      // once, so that we don't have to store so many alignment vectors
      vector<pair<size_t, vector<uchar> > > my_aln; // internal word alignment

      uint32_t ofwd[Moses::LRModel::NONE+1]; //  forward distortion type counts
      uint32_t obwd[Moses::LRModel::NONE+1]; // backward distortion type counts

    public:
      vector<uint32_t> indoc; // counts origin of samples (for biased sampling)
      jstats();
      jstats(jstats const& other);
      uint32_t rcnt() const; // raw joint counts
      uint32_t cnt2() const; // raw target phrase occurrence count
      float    wcnt() const; // weighted joint counts
      
      vector<pair<size_t, vector<uchar> > > const & aln() const;
      void add(float w, vector<uchar> const& a, uint32_t const cnt2,
	       uint32_t fwd_orient, uint32_t bwd_orient, 
	       int const docid);
      void invalidate();
      void validate();
      bool valid();
      uint32_t dcnt_fwd(PhraseOrientation const idx) const;
      uint32_t dcnt_bwd(PhraseOrientation const idx) const;
      void fill_lr_vec(Moses::LRModel::Direction const& dir, 
		       Moses::LRModel::ModelType const& mdl, 
		       vector<float>& v);
    };
  }
}

// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once
#include <string>
#include <stdint.h>
#include "ug_typedefs.h"
#include "ug_lexical_reordering.h"
#include <boost/thread.hpp>

namespace sapt
{

  // "joint" (i.e., phrase std::pair) statistics
  class
  jstats
  {
    boost::mutex lock;
    uint32_t my_rcnt; // unweighted joint count
    uint32_t my_cnt2; // raw counts L2
    float    my_wcnt; // weighted joint count
    float    my_bcnt; // cumulative bias 

    // to do: use a static alignment pattern store that stores each pattern only
    // once, so that we don't have to store so many alignment std::vectors
    std::vector<std::pair<size_t, std::vector<unsigned char> > > my_aln; 
    // internal word alignment

    uint32_t ofwd[LRModel::NONE+1]; //  forward distortion type counts
    uint32_t obwd[LRModel::NONE+1]; // backward distortion type counts

  public:
    SPTR<std::vector<uint32_t> > sids; // list of sentence ids in this sample
    std::map<uint32_t,uint32_t> indoc;
    // std::vector<uint32_t> indoc; // counts origin of samples (for biased sampling)
    jstats();
    jstats(jstats const& other);
    uint32_t rcnt() const; // raw joint counts
    uint32_t cnt2() const; // raw target phrase occurrence count
    float    wcnt() const; // weighted joint counts
    float    bcnt() const; // cumulative bias scores

    std::vector<std::pair<size_t, std::vector<unsigned char> > > const & aln() const;

    size_t 
    add(float w, float b, std::vector<unsigned char> const& a, uint32_t const cnt2, 
	uint32_t fwd_orient, uint32_t bwd_orient, int const docid, uint32_t const sid,
	bool const track_sid);

    void invalidate();
    void validate();
    bool valid();
    uint32_t dcnt_fwd(PhraseOrientation const idx) const;
    uint32_t dcnt_bwd(PhraseOrientation const idx) const;
    void fill_lr_vec(LRModel::Direction const& dir,
                     LRModel::ModelType const& mdl,
                     std::vector<float>& v);
  };
}


// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil -*-
#pragma once
#include <vector>
#include "ug_typedefs.h"

namespace sapt 
{
  struct PhraseExtractionRecord
  {
    size_t const  sid, start, stop;
    bool   const        flip; // 'backward' lookup from L2
    size_t    s1, s2, e1, e2; // soft and hard boundaries of target phrase
    int       po_fwd, po_bwd; // fwd and bwd phrase orientation 
    std::vector<unsigned char>*  aln; // local alignments
    bitvector*      full_aln; // full word alignment for sentence
    
    PhraseExtractionRecord(size_t const xsid, size_t const xstart, 
                           size_t const xstop, bool const xflip,
                           std::vector<unsigned char>* xaln, 
                           bitvector* xfull_aln = NULL)
      : sid(xsid), start(xstart), stop(xstop), flip(xflip) 
      , aln(xaln), full_aln(xfull_aln) { }
  };
}


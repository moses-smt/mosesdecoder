// -*- c++ -*-
#pragma once
#include <vector>

#ifndef NO_MOSES
#include "moses/FF/LexicalReordering/LexicalReorderingState.h"
#endif

namespace Moses { 
#ifdef NO_MOSES
namespace LRModel{

  enum ModelType { Monotonic, MSD, MSLR, LeftRight, None };
  enum Direction { Forward, Backward, Bidirectional };

  enum ReorderingType {
    M    = 0, // monotonic
    NM   = 1, // non-monotonic
    S    = 1, // swap
    D    = 2, // discontinuous
    DL   = 2, // discontinuous, left
    DR   = 3, // discontinuous, right
    R    = 0, // right
    L    = 1, // left
    MAX  = 3, // largest possible
    NONE = 4  // largest possible
  };

}
#endif

namespace bitext {

typedef Moses::LRModel::ReorderingType PhraseOrientation;

PhraseOrientation
find_po_fwd(std::vector<std::vector<ushort> >& a1,
	    std::vector<std::vector<ushort> >& a2,
	    size_t b1, size_t e1,
	    size_t b2, size_t e2);

PhraseOrientation
find_po_bwd(std::vector<std::vector<ushort> >& a1,
	    std::vector<std::vector<ushort> >& a2,
	    size_t b1, size_t e1,
	    size_t b2, size_t e2);




}} // close namespaces

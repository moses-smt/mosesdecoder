// -*- c++ -*-
#pragma once
#include <vector>
#include "moses/FF/LexicalReordering/LexicalReorderingState.h"

namespace Moses { namespace bitext {

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

// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "ug_tsa_array_entry.h"
#include "ug_ttrack_position.h"
#include "moses/TranslationModel/UG/generic/sampling/Sampling.h"

// (c) 2007-2010 Ulrich Germann

namespace sapt
{
  namespace tsa
  {
    ArrayEntry::ArrayEntry() : ttrack::Position(0,0), pos(NULL), next(NULL) {};
    ArrayEntry::ArrayEntry(char const* p) : ttrack::Position(0,0), pos(NULL), next(p) {};

  }
}

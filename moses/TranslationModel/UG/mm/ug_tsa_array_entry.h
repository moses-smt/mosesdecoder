// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2007-2010 Ulrich Germann
// implementation of stuff related to ArrayEntries
// this file should only be included via ug_tsa_base.h,
// never by itself
#ifndef __ug_tsa_array_entry_h
#define __ug_tsa_array_entry_h
#include "ug_ttrack_position.h"

namespace sapt
{
  namespace tsa
  {
    class
    ArrayEntry : public ttrack::Position
    {
    public:
      char const* pos;
      char const* next;
      ArrayEntry();

      ArrayEntry(char const* p);

      template<typename TSA_TYPE>
      ArrayEntry(TSA_TYPE const* S, char const* p);

    };

    template<typename TSA_TYPE>
    ArrayEntry::
    ArrayEntry(TSA_TYPE const* S, char const* p)
    {
      S->readEntry(p,*this);
    }

  } // end of namespace tsa
} // end of namespace sapt
#endif

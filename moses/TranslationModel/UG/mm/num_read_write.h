// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2006,2007,2008 Ulrich Germann
// #ifndef __num_read_write_hh
// #define __num_read_write_hh
#pragma once
#include <stdint.h>
#include <iostream>
// #include <endian.h>
// #include <byteswap.h>
// #include "tpt_typedefs.h"

namespace tpt {

  void numwrite(std::ostream& out, uint16_t const& x);
  void numwrite(std::ostream& out, uint32_t const& x);
  void numwrite(std::ostream& out, uint64_t const& x);

  char const* numread(char const* src, uint16_t & x);
  char const* numread(char const* src, uint32_t & x);
  char const* numread(char const* src, uint64_t & x);

} // end of namespace ugdiss


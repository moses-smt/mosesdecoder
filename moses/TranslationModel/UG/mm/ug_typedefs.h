// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// typedefs for Uli Germann's stuff
#ifndef __ug_typedefs_h
#define __ug_typedefs_h
#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>
#include <stdint.h>
#include "tpt_typedefs.h"
namespace sapt
{
  typedef boost::dynamic_bitset<uint64_t> bitvector;

  typedef std::vector<std::vector<float> > flt_2d_table;
  typedef std::vector<flt_2d_table>        flt_3d_table;
  typedef std::vector<flt_3d_table>        flt_4d_table;

  typedef std::vector<std::vector<ushort> > ushort_2d_table;
  typedef std::vector<ushort_2d_table>      ushort_3d_table;
  typedef std::vector<ushort_3d_table>      ushort_4d_table;

  typedef std::vector<std::vector<short> > short_2d_table;
  typedef std::vector<short_2d_table>      short_3d_table;
  typedef std::vector<short_3d_table>      short_4d_table;

  typedef std::vector<std::vector<int> > int_2d_table;
  typedef std::vector<int_2d_table>      int_3d_table;
  typedef std::vector<int_3d_table>      int_4d_table;

  typedef tpt::id_type id_type;
  typedef tpt::uchar uchar;
  typedef tpt::filepos_type filepos_type;
}

#ifndef SPTR
#define SPTR   boost::shared_ptr
#endif
#define boost_iptr   boost::intrusive_ptr
#define scoptr boost::scoped_ptr
#define rcast  reinterpret_cast
#endif

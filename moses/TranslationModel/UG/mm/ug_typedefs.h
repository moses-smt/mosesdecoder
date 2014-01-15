// -*- c++ -*-
// typedefs for Uli Germann's stuff
#ifndef __ug_typedefs_h
#define __ug_typedefs_h
#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>
#include <stdint.h>
#include "tpt_typedefs.h"
namespace ugdiss
{
  using namespace std;
  typedef boost::dynamic_bitset<uint64_t> bitvector;

  typedef vector<vector<float> >     flt_2d_table;
  typedef vector<flt_2d_table>       flt_3d_table;
  typedef vector<flt_3d_table>       flt_4d_table;

  typedef vector<vector<ushort> > ushort_2d_table;
  typedef vector<ushort_2d_table> ushort_3d_table;
  typedef vector<ushort_3d_table> ushort_4d_table;

  typedef vector<vector<short> >   short_2d_table;
  typedef vector<short_2d_table>   short_3d_table;
  typedef vector<short_3d_table>   short_4d_table;
  
  typedef vector<vector<int> >       int_2d_table;
  typedef vector<int_2d_table>       int_3d_table;
  typedef vector<int_3d_table>       int_4d_table;
}

#define sptr   boost::shared_ptr
#define scoptr boost::scoped_ptr
#define rcast  reinterpret_cast
#endif

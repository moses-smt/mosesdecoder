// -*- c++ -*-
// (c) 2007,2008 Ulrich Germann
/* Functions for writing indices tightly (use only the bytes you need).
 * The first bit indicates whether a byte belongs to a key or a value.
 * The remaining 7 bits are part of the respective integer value.
 */
#ifndef __ugTightIndex
#define __ugTightIndex
#include <map>
#include <iostream>
#include <sstream>
#include "tpt_typedefs.h"
#include <cassert>
// using namespace std;

#ifndef uchar
#endif

#define FLAGBITS 2
#define FLAGMASK (uchar(3))
#define HAS_VALUE_MASK (uchar(2))
#define HAS_CHILD_MASK (uchar(1))


extern bool debug;

namespace ugdiss
{
  // void tightwritex(iostream& out, size_t data, bool flag);
  void 
  tightwrite(std::ostream& out, uint64_t data, bool flag);

  filepos_type 
  tightread(std::istream& in, std::ios::pos_type stop);

  bool
  tightfind(std::istream& in, 
	    filepos_type start, 
	    filepos_type stop, 
	    id_type key,
	    unsigned char& flags);

  bool
  tightfind_noflags(std::istream& in, 
                    filepos_type start, 
                    filepos_type stop, 
                    id_type key);

  char const*
  tightfind(char const* const start, 
            char const* const stop,
            id_type key, 
            unsigned char& flags);

  char const*
  tightfind_noflags(char const* const start, 
                    char const* const stop,
                    id_type key);



  /** move read header in istream /in/ to the first entry after the midpoint of 
   *  file position range [start,stop) in in a 'tight' index 
   *  @param in the data input stream
   *  @param start start of the search range
   *  @param stop  end   of the search range
   *  @return true if no errors occurred 
   */ 
  bool 
  tightfind_midpoint(std::istream& in, filepos_type start, filepos_type stop);

  // the bitpattern functions below are for debugging
  // They return a string showing the bits of the argument value
//   std::string bitpattern(unsigned int s);
//   std::string bitpattern(unsigned char c);
//   std::string bitpattern(char c);


  /** read a number from a tight index directy from a memory location
   *  @param start start of read range
   *  @param stop  non-inclusive end of read range
   *  @param dest  destination
   *  @return first memory position after the number
   */

  char const*
  tightread2(char const* start, char const* stop, uint16_t& dest);

  char const*
  tightread4(char const* start, char const* stop, uint32_t& dest);

  char const*
  tightread8(char const* start, char const* stop, uint64_t& dest);

  template<typename numType>
  char const*
  tightread(char const* start, char const* stop, numType& dest)
  {
    if (sizeof(numType)==2)
      return tightread2(start,stop,reinterpret_cast<uint16_t&>(dest));
    if (sizeof(numType)==4)
      return tightread4(start,stop,reinterpret_cast<uint32_t&>(dest));
    else if (sizeof(numType)==8)
      return tightread8(start,stop,reinterpret_cast<uint64_t&>(dest));
    assert(0);
    return NULL;
  }

//   char const*
//   tightread(char const* start, char const* stop, uint64_t& dest);

//   char const*
//   tightread(char const* start, char const* stop, filepos_type& dest);

#if 0
  template<typename dtype>
  char const* 
  tightread(char const* start, 
            char const* stop,
            dtype& dest)
  {
    static char bitmask=127;
    dest = 0;
    if (*start < 0)
      {
        dest = (*start)&bitmask;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<7;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<14;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<21;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<28;
        if (++start==stop || *start >= 0) return start;
        assert(sizeof(dtype) > 4);
        dest += dtype((*start)&bitmask)<<35;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<42;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<49;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<56;
        if (++start==stop || *start >= 0) return start;
        dest += dtype((*start)&bitmask)<<63;
      }
    else
      {
        dest = *start;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<7;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<14;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<21;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<28;
        if (++start==stop || *start < 0) return start;
        assert(sizeof(dtype) > 4);
        dest += dtype(*start)<<35;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<42;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<49;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<56;
        if (++start==stop || *start < 0) return start;
        dest += dtype(*start)<<63;
      }
    assert(start<stop);
    return ++start;
  }
#endif


}
#endif

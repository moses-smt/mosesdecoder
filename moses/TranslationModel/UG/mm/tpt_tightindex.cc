// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2007,2008 Ulrich Germann

/* Functions for writing indices tightly (use only the bytes you need).
 * The first bit indicates whether a byte belongs to a key or a value.
 * The remaining 7 bits are part of the respective integer value.
 * (c) 2007 Ulrich Germann
 */
//
// ugTightIndex.cc
//
// Made by Ulrich Germann
// Login   <germann@germann-laptop>
//
// Started on  Tue Jul 17 15:09:33 2007 Ulrich Germann
// Started on  Tue Jul 17 15:09:33 2007 Ulrich Germann
//

#include <iostream>
#include <cassert>
#include "tpt_tightindex.h"

namespace tpt
{

  // #define LOG_WRITE_ACTIVITY

  // write a key or value into a tight index
  // flag indicates wheter it's a key or a value
  void tightwrite(std::ostream& out, uint64_t data, bool flag)
  {
    // assert(sizeof(size_t)==4);
#ifdef LOG_WRITE_ACTIVITY
    size_t bytes_written=1;
    std::cerr << "starting at file position " << out.tellp()
	      << ": tightwrite " << data;
#endif
    if (flag)
      {
#ifdef LOG_WRITE_ACTIVITY
	std::cerr << " with flag 1 ";
#endif
	while (data >= 128)
	  {
	    char c = char(data%128)|char(-128);
	    out.put(c);
	    data >>= 7;
#ifdef LOG_WRITE_ACTIVITY
	    bytes_written++;
#endif
	  }
	char c = char(data%128)|char(-128);
	out.put(c);
      }
    else
      {
#ifdef LOG_WRITE_ACTIVITY
	std::cerr << " with flag 0 ";
#endif
	while (data >= 128)
	  {
	    char c = data&127;
	    out.put(c);
	    data >>= 7;
#ifdef LOG_WRITE_ACTIVITY
	    bytes_written++;
#endif
	  }
	char c = (data&127);
	out.put(c);
      }
#ifdef LOG_WRITE_ACTIVITY
    std::cerr << " in " << bytes_written << " bytes" << std::endl;
#endif
  }

// For the code below: does it make a difference if I hard-code the
// unraveled loop or does code optimization by the compiler take care
// of that?

#define DEBUG_TIGHTREAD 0

  // read a key value from a tight index; filepos_type must be at least as
  // large as count_type
  filepos_type
  tightread(std::istream& in, std::ios::pos_type stop)
  {
    // debug=true;
    // assert(sizeof(size_t) == 4);
    assert(in.rdbuf()->in_avail() > 0);
    filepos_type data     = 0;
    short int bitshift = 7;
    int pos = in.tellg();
#if DEBUG_TIGHTREAD
    if (debug)
      cerr << bitpattern(uint(in.peek())) << " " << in.peek()
	   << " pos=" << in.tellg() << "\n";
#endif
    int buf = in.get();
    if (stop == std::ios::pos_type(0))
      stop = size_t(in.tellg())+in.rdbuf()->in_avail();
    else
      stop = std::min(size_t(stop),size_t(in.tellg())+in.rdbuf()->in_avail());
    if (buf < 0)
      std::cerr << "number read: " << buf << " " << pos << " "
		<< in.tellg() << std::endl;
    assert (buf>=0);

    if (buf >= 128) // continuation bit is 1
      {
	data = buf-128; // unset the bit
	while (in.tellg() < stop && in.peek() >= 128)
	  {
#if DEBUG_TIGHTREAD
	    if (debug)
	      cerr << bitpattern(uint(in.peek())) << " " << in.peek();
#endif
	    // cerr << bitpattern(size_t(in.peek())) << std::endl;
	    data += size_t(in.get()-128)<<bitshift;
	    bitshift += 7;
#if DEBUG_TIGHTREAD
	    if (debug)
	      cerr << " " << data << " pos=" << in.tellg() << std::endl;
#endif
	  }
      }
    else
      {
	data = buf;
	while (in.tellg() < stop && in.peek() < 128)
	  {
	    // cerr << bitpattern(size_t(in.peek())) << std::endl;
#if DEBUG_TIGHTREAD
	    if (debug)
	      cerr << bitpattern(uint(in.peek())) << " " << in.peek();

#endif
	    data += size_t(in.get())<<bitshift;
	    bitshift += 7;
#if DEBUG_TIGHTREAD
	    if (debug)
	      cerr << " " << data << " pos=" << in.tellg() << "\n";
#endif
	  }
      }
    return data;
  }

#define DEBUG_TIGHTFIND 0
#if DEBUG_TIGHTFIND
bool debug=true;
#endif
  bool
  tightfind_midpoint(std::istream& in, filepos_type start, filepos_type stop)
  {
    in.seekg((start+stop)/2);
    // Jump approximately to the middle. Since we might land in the
    // middle of a number, we need to find the start of the next
    // [index key/file offset] pair first. Bytes belonging to an index
    // key have the leftmost bit set to 0, bytes belonging to a file
    // offset have it set to 1

    // if we landed in the middle of an index key, skip to the end of it
    while (static_cast<filepos_type>(in.tellg()) < stop && in.get() < 128)
      {
#if DEBUG_TIGHTFIND
	if (debug)
	  {
	    in.unget();
	    char c = in.get();
	    std::cerr << in.tellg() << " skipped key byte " << c << std::endl;
	  }
#endif
	if (in.eof()) return false;
      }
  // Also skip the associated file offset:
    while (static_cast<filepos_type>(in.tellg()) < stop && in.peek() >= 128)
      {
#if DEBUG_TIGHTFIND
	int r = in.get();
	if (debug)
	  std::cerr << in.tellg() << " skipped value byte " << r
	       << " next is " << in.peek()
	       << std::endl;
#else
	in.get();
#endif
      }
    return true;
  }

  char const*
  tightfind_midpoint(char const* const start,
                     char const* const stop)
  {
    char const* mp = start + (stop - start)/2;
    while (*mp < 0  && mp > start) mp--;
    while (*mp >= 0 && mp > start) mp--;
    return (*mp < 0) ? ++mp : mp;
  }

  bool
  linear_search(std::istream& in, filepos_type start, filepos_type stop,
		id_type key, unsigned char& flags)
  { // performs a linear search in the range
    in.seekg(start);

#if DEBUG_TIGHTFIND
    if (debug) std::cerr << in.tellg() << " ";
#endif

    // ATTENTION! The bitshift operations below are important:
    // We use some of the bits in the key value to store additional
    // information about what and where node iformation is stored.

    id_type foo;
    for(foo = tightread(in,stop);
	(foo>>FLAGBITS) < key;
      foo = tightread(in,stop))
      {
	// skip the value associated with key /foo/
	while (static_cast<filepos_type>(in.tellg()) < stop
	       && in.peek() >= 128) in.get();

#if DEBUG_TIGHTFIND
	if (debug)
	  std::cerr << (foo>>FLAGBITS) << " [" << key << "] "
	       << in.tellg() << std::endl;
#endif

	if (in.tellg() == std::ios::pos_type(stop))
	  return false; // not found
      }

#if DEBUG_TIGHTFIND
    if (debug && (foo>>FLAGBITS)==key)
      std::cerr << "found entry for " << key << std::endl;
    std::cerr << "current file position is " << in.tellg()
              << " (value read: " << key << std::endl;
#endif

    assert(static_cast<filepos_type>(in.tellg()) < stop);
    if ((foo>>FLAGBITS)==key)
      {
	flags = (foo%256);
	flags &= FLAGMASK;
	return true;
      }
    else
      return false;
  }

  bool
  tightfind(std::istream& in, filepos_type start, filepos_type stop,
	    id_type key, unsigned char& flags)
  {
    // returns true if the value is found
#if DEBUG_TIGHTFIND
    if (debug)
      std::cerr << "looking for " << key
	   << " in range [" << start << ":" << stop << "]" << std::endl;
#endif
    if (start==stop) return false;
    assert(stop>start);
    if ((start+1)==stop) return false; // list is empty

    unsigned int const granularity = sizeof(filepos_type)*5;
    // granularity: point where we should switch to linear search,
    // because otherwise we might skip over the entry we are looking for
    // because we land right in the middle of it.

    if (stop > start + granularity)
      if (!tightfind_midpoint(in,start,stop))
	return false; // something went wrong (empty index)

    if (stop <= start + granularity || in.tellg() == std::ios::pos_type(stop))
      { // If the search range is very short, tightfind_midpoint might skip the
	// entry we are loking for. In this case, we can afford a linear
	// search
	return linear_search(in,start,stop,key,flags);
      }

    // perform binary search
    filepos_type curpos = in.tellg();
    id_type foo = tightread(in,stop);
    id_type tmpid = foo>>FLAGBITS;
    if (tmpid == key)
      {
	flags  = foo%256;
	flags &= FLAGMASK;
#if DEBUG_TIGHTFIND
	if (debug) std::cerr << "found entry for " << key << std::endl;
#endif
	return true; // done, found
      }
    else if (tmpid > key)
      { // look in the lower half
#if DEBUG_TIGHTFIND
	if (debug) std::cerr << foo << " > " << key << std::endl;
#endif
	return tightfind(in,start,curpos,key,flags);
      }
    else
      { // look in the upper half
	while (static_cast<filepos_type>(in.tellg()) < stop
	       && in.rdbuf()->in_avail() > 0 // is that still necessary???
	       && in.peek() >= 128)
	  in.get(); // skip associated value
	if (in.rdbuf()->in_avail() == 0 || in.tellg() == std::ios::pos_type(stop))
	  return false;
#if DEBUG_TIGHTFIND
	if (debug) std::cerr << foo << " < " << key << std::endl;
#endif
	return tightfind(in,in.tellg(),stop,key,flags);
      }
  }


  char const*
  tightfind(char const* const start,
            char const* const stop,
	    id_type key,
            unsigned char& flags)
  {
    // returns true if the value is found

    if (start==stop) return NULL;
    assert(stop>start);
    if ((start+1)==stop) return NULL; // list is empty
    char const* p = tightfind_midpoint(start,stop);
    // if ids can be larger than 67,108,864 on 32-bit machines
    // (i.e., 2**(28-flagbits)), dest must be declared as uint64_t
    size_t foo;
    char const* after = tightread(p,stop,foo);
    id_type tmpId = foo>>FLAGBITS;
    if (tmpId == key)
      {
	flags  = foo%256;
	flags &= FLAGMASK;
        return after;
      }
    else if (tmpId > key)
      { // look in the lower half
	return tightfind(start,p,key,flags);
      }
    else
      { // look in the upper half
        while (*after<0 && ++after < stop);
        if (after == stop) return NULL;
	return tightfind(after,stop,key,flags);
      }
  }

  char const*
  tightfind_noflags(char const* const start,
                    char const* const stop,
                    id_type key)
  {
    // returns true if the value is found

    if (start==stop) return NULL;
    assert(stop>start);
    if ((start+1)==stop) return NULL; // list is empty
    char const* p = tightfind_midpoint(start,stop);
    // if ids can be larger than 67,108,864 on 32-bit machines
    // (i.e., 2**(28-flagbits)), dest must be declared as uint64_t
    size_t foo;
    char const* after = tightread(p,stop,foo);
    if (foo == key)
      return after;
    else if (foo > key)
      { // look in the lower half
	return tightfind_noflags(start,p,key);
      }
    else
      { // look in the upper half
        while (*after<0 && ++after < stop);
        if (after == stop) return NULL;
	return tightfind_noflags(after,stop,key);
      }
  }

  bool
  linear_search_noflags(std::istream& in, filepos_type start,
                filepos_type stop, id_type key)
  { // performs a linear search in the range
    std::ios::pos_type mystop = stop;

    in.seekg(start);
    id_type foo;
    for(foo = tightread(in,stop); foo < key; foo = tightread(in,stop))
      {
	// skip the value associated with key /foo/
	while (in.tellg() < mystop && in.peek() >= 128)
          in.get();
	if (in.tellg() == mystop)
	  return false; // not found
      }
    assert(in.tellg() < mystop);
    return (foo==key);
  }


  bool
  tightfind_noflags(std::istream& in, filepos_type start,
                    filepos_type stop, id_type key)
  {
    // returns true if the value is found
    if (start==stop) return false;
    assert(stop>start);
    if ((start+1)==stop) return false; // list is empty

    // granularity: point where we should switch to linear search,
    // because otherwise we might skip over the entry we are looking for
    // because we land right in the middle of it.
    unsigned int const granularity = sizeof(filepos_type)*5;
    // UG: why 5? we should be able to get away with less!

    if (stop > start + granularity)
      if (!tightfind_midpoint(in,start,stop))
	return false; // something went wrong (empty index)

    // If the search range is very short, tightfind_midpoint might skip the
    // entry we are loking for. In this case, we can afford a linear
    // search
    if (stop <= start + granularity || in.tellg() == std::ios::pos_type(stop))
      return linear_search_noflags(in,start,stop,key);

    // Otherwise, perform binary search
    filepos_type curpos = in.tellg();
    id_type foo = tightread(in,stop);
    if (foo == key)
      return true; // done, found

    else if (foo > key) // search first half
      return tightfind_noflags(in,start,curpos,key);

    else // search second half
      {
        std::ios::pos_type mystop = stop;
	while (in.tellg() < mystop
	       && in.rdbuf()->in_avail() > 0 // is that still necessary???
	       && in.peek() >= 128)
	  in.get(); // skip associated value
	if (in.rdbuf()->in_avail() == 0 || in.tellg() == mystop)
	  return false;
	return tightfind_noflags(in,in.tellg(),stop,key);
      }
  }

  void tightwrite2(std::ostream& out, size_t data, bool flag)
  {
    // same as tightwrite, but uses basic storage units of size 2
    // assert(sizeof(size_t)==4);
    short int foo = (data%32768);
    if (flag)
      {
	foo += 32768; // set first bit
	while (data >= 32768) // = 2^15
	  {
	    out.write(reinterpret_cast<char*>(&foo),2);
	    data >>= 15;
	    foo = (data%32768)+32768;
	  }
      }
    else
      {
	while (data >= 32768) // = 2^15
	  {
	    out.write(reinterpret_cast<char*>(&foo),2);
	    data >>= 15;
	    foo = data%32768;
	  }
      }
    out.write(reinterpret_cast<char*>(&foo),2);
  }

  char const*
  tightread8(char const* start,
             char const* stop,
             uint64_t& dest)
  {
    static char bitmask=127;
    dest = 0;
    if (*start < 0)
      {
        dest = (*start)&bitmask;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<7;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<14;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<21;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<28;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<35;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<42;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<49;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<56;
        if (++start==stop || *start >= 0) return start;
        dest += uint64_t((*start)&bitmask)<<63;
      }
    else
      {
        dest = *start;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<7;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<14;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<21;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<28;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<35;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<42;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<49;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<56;
        if (++start==stop || *start < 0) return start;
        dest += uint64_t(*start)<<63;
      }
    assert(start<stop);
    return ++start;
  }

  char const*
  tightread4(char const* start,
             char const* stop,
             uint32_t& dest)
  {
    static char bitmask=127;
    dest = 0;
    if (*start < 0)
      {
        dest = (*start)&bitmask;
        if (++start==stop || *start >= 0) return start;
        dest += uint32_t((*start)&bitmask)<<7;
        if (++start==stop || *start >= 0) return start;
        dest += uint32_t((*start)&bitmask)<<14;
        if (++start==stop || *start >= 0) return start;
        dest += uint32_t((*start)&bitmask)<<21;
        if (++start==stop || *start >= 0) return start;
        dest += uint32_t((*start)&bitmask)<<28;
      }
    else
      {
        dest = *start;
        if (++start==stop || *start < 0) return start;
        dest += uint32_t(*start)<<7;
        if (++start==stop || *start < 0) return start;
        dest += uint32_t(*start)<<14;
        if (++start==stop || *start < 0) return start;
        dest += uint32_t(*start)<<21;
        if (++start==stop || *start < 0) return start;
        dest += uint32_t(*start)<<28;
      }
    assert(start<stop);
    return ++start;
  }

  char const*
  tightread2(char const* start,
             char const* stop,
             uint16_t& dest)
  {
    static char bitmask=127;
    dest = 0;
    if (*start < 0)
      {
        dest = (*start)&bitmask;
        if (++start==stop || *start >= 0) return start;
        dest += uint32_t((*start)&bitmask)<<7;
        if (++start==stop || *start >= 0) return start;
        dest += uint32_t((*start)&bitmask)<<14;
      }
    else
      {
        dest = *start;
        if (++start==stop || *start < 0) return start;
        dest += uint32_t(*start)<<7;
        if (++start==stop || *start < 0) return start;
        dest += uint32_t(*start)<<14;
      }
    assert(start<stop);
    return ++start;
  }
} // end namespace ugdiss

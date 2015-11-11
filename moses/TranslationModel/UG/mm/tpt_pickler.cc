// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2006,2007,2008 Ulrich Germann

#include "tpt_pickler.h"
#include <sys/stat.h>
#include <cassert>

#ifdef __CYGWIN__
#define stat64  stat
#endif

namespace tpt
{

  uint64_t
  getFileSize(const std::string& fname)
  {
    struct stat64 buf;
    stat64(fname.c_str(),&buf);
    return buf.st_size;
  }

  template <typename T>
  void
  binwrite_unsigned_integer(std::ostream& out, T data)
  {
    char c;
    while (data >= 128)
      {
	out.put(data%128);
	data = data >> 7;
      }
    c = data;
    out.put(c|char(-128)); // set the 'stop' bit
  }

  template<typename T>
  void
  binread_unsigned_integer(std::istream& in, T& data)
  {
    char c, mask=127;
    in.clear();
    in.get(c);
    data = c&mask;

    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 28;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 35;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 42;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 49;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 56;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 63;
  }

  void
  binwrite(std::ostream& out, unsigned char data)
  {
    binwrite_unsigned_integer(out, data);
  }

  void
  binwrite(std::ostream& out, unsigned short data)
  {
    binwrite_unsigned_integer(out, data);
  }

  void
  binwrite(std::ostream& out, unsigned long data)
  {
    binwrite_unsigned_integer(out, data);
  }

  void
  binwrite(std::ostream& out, unsigned long long data)
  {
    binwrite_unsigned_integer(out, data);
  }

#if __WORDSIZE == 64
  void
  binwrite(std::ostream& out, unsigned int data)
  {
    binwrite_unsigned_integer(out, data);
  }
#else
  void
  binwrite(std::ostream& out, size_t data)
  {
    binwrite_unsigned_integer(out, data);
  }
#endif

  void
  binread(std::istream& in, unsigned short& data)
  {
    assert(sizeof(data)==2);
    char c, mask=127;
    in.clear();
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += uint16_t(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += uint16_t(c&mask) << 14;
  }

  void
  binread(std::istream& in, unsigned int& data)
  {
    assert(sizeof(data) == 4);
    char c, mask=127;
    in.clear();
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 28;
  }

  void
  binread(std::istream& in, unsigned long& data)
  {
#if __WORDSIZE == 32
    assert(sizeof(unsigned long)==4);
#else
    assert(sizeof(unsigned long)==8);
#endif
    char c, mask=127;
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 28;
#if __WORDSIZE == 64
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 35;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 42;
    if (c < 0) return;
    in.get(c);

    data += static_cast<unsigned long long>(c&mask) << 49;
    if (c < 0) return;
    in.get(c);

    data += static_cast<unsigned long long>(c&mask) << 56;
    if (c < 0) return;
    in.get(c);

    data += static_cast<unsigned long long>(c&mask) << 63;
#endif
  }

  void
  binread(std::istream& in, unsigned long long& data)
  {
    assert(sizeof(unsigned long long)==8);
    char c, mask=127;
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 28;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 35;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 42;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 49;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 56;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 63;
  }

  // writing and reading strings ...
  void
  binwrite(std::ostream& out, std::string const& s)
  {
    size_t len = s.size();
    binwrite(out,len);
    out.write(s.c_str(),len);
  }

  void
  binread(std::istream& in, std::string& s)
  {
    size_t len;
    binread(in,len);
    if (!in) return;
    char buf[len+1];
    in.read(buf,len);
    buf[len] = 0;
    s = buf;
  }

  void
  binwrite(std::ostream& out, float x)
  {
    // IMPORTANT: this is not robust against the big/little endian
    // issue.
    out.write(reinterpret_cast<char*>(&x),sizeof(float));
  }

  void
  binread(std::istream& in, float& x)
  {
    // IMPORTANT: this is not robust against the big/little endian
    // issue.
    in.read(reinterpret_cast<char*>(&x),sizeof(x));
  }


  char const *binread(char const* p, uint16_t& buf)
  {
    static char mask = 127;
    buf = (*p)&mask;
    if (*p++ < 0) return p;
    buf += uint16_t((*p)&mask)<<7;
    if (*p++ < 0) return p;
    buf += uint16_t((*p)&mask)<<14;
#ifndef NDEBUG
    assert(*p++ < 0);
#else
    ++p;
#endif
    return p;
  }

#ifdef __clang__
  char const *binread(char const* p, size_t& buf)
  {
	  return binread(p, (uint32_t&) buf);
  }
#endif

  char const *binread(char const* p, uint32_t& buf)
  {
    static char mask = 127;

    if (*p < 0)
      {
        buf = (*p)&mask;
        return ++p;
      }
    buf = *p;
    if (*(++p) < 0)
      {
        buf += uint32_t((*p)&mask)<<7;
        return ++p;
      }
    buf += uint32_t(*p)<<7;
    if (*(++p) < 0)
      {
        buf += uint32_t((*p)&mask)<<14;
        return ++p;
      }
    buf += uint32_t(*p)<<14;
    if (*(++p) < 0)
      {
        buf += uint32_t((*p)&mask)<<21;
        return ++p;
      }
    buf += uint32_t(*p)<<21;
#ifndef NDEBUG
    assert(*(++p) < 0);
#else
    ++p;
#endif
    buf += uint32_t((*p)&mask)<<28;
    return ++p;
  }

  char const *binread(char const* p, filepos_type& buf)
  {
    static char mask = 127;

    if (*p < 0)
      {
        buf = (*p)&mask;
        return ++p;
      }
    buf = *p;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<7;
        return ++p;
      }
    buf += filepos_type(*p)<<7;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<14;
        return ++p;
      }
    buf += filepos_type(*p)<<14;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<21;
        return ++p;
      }
    buf += filepos_type(*p)<<21;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<28;
        return ++p;
      }
    buf += filepos_type(*p)<<28;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<35;
        return ++p;
      }
    buf += filepos_type(*p)<<35;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<42;
        return ++p;
      }
    buf += filepos_type(*p)<<42;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<49;
        return ++p;
      }
    buf += filepos_type(*p)<<49;
    if (*(++p) < 0)
      {
        buf += filepos_type((*p)&mask)<<56;
        return ++p;
      }
    buf += filepos_type(*p)<<56;
#ifndef NDEBUG
    assert(*(++p) < 0);
#else
    ++p;
#endif
    buf += filepos_type((*p)&mask)<<63;
    return ++p;
  }

  char const *binread(char const* p, float& buf)
  {
    buf = *reinterpret_cast<float const*>(p);
    return p+sizeof(float);
  }

} // end namespace ugdiss

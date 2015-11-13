#include "num_read_write.h"
namespace tpt {
  typedef unsigned char uchar;

  void
  numwrite(std::ostream& out, uint16_t const& x)
  {
    char buf[2];
    buf[0] = x%256;
    buf[1] = (x>>8)%256;
    out.write(buf,2);
  }

  void
  numwrite(std::ostream& out, uint32_t const& x)
  {
    char buf[4];
    buf[0] = x%256;
    buf[1] = (x>>8)%256;
    buf[2] = (x>>16)%256;
    buf[3] = (x>>24)%256;
    out.write(buf,4);
  }

  void
  numwrite(std::ostream& out, uint64_t const& x)
  {
    char buf[8];
    buf[0] = x%256;
    buf[1] = (x>>8)%256;
    buf[2] = (x>>16)%256;
    buf[3] = (x>>24)%256;
    buf[4] = (x>>32)%256;
    buf[5] = (x>>40)%256;
    buf[6] = (x>>48)%256;
    buf[7] = (x>>56)%256;
    out.write(buf,8);
  }

  char const*
  numread(char const* src, uint16_t & x)
  {
    uchar const* d = reinterpret_cast<uchar const*>(src);
    x = (uint16_t(d[0])<<0) | (uint16_t(d[1])<<8);
    return src+2;
  }

  char const*
  numread(char const* src, uint32_t & x)
  {
    uchar const* d = reinterpret_cast<uchar const*>(src);
    x = ((uint32_t(d[0])<<0) |
	 (uint32_t(d[1])<<8) |
	 (uint32_t(d[2])<<16)|
	 (uint32_t(d[3])<<24));
    return src+4;
  }

  char const*
  numread(char const* src, uint64_t & x)
  {
    uchar const* d = reinterpret_cast<uchar const*>(src);
    x = ((uint64_t(d[0])<<0)  |
	 (uint64_t(d[1])<<8)  |
	 (uint64_t(d[2])<<16) |
	 (uint64_t(d[3])<<24) |
	 (uint64_t(d[4])<<32) |
	 (uint64_t(d[5])<<40) |
	 (uint64_t(d[6])<<48) |
	 (uint64_t(d[7])<<56));
    return src+8;
  }

}

#ifndef MERT_GZFILEBUF_H_
#define MERT_GZFILEBUF_H_

#include <streambuf>
#include <zlib.h>

class GzFileBuf : public std::streambuf
{
public:
  explicit GzFileBuf(const char* filename);
  virtual ~GzFileBuf();

protected:
  virtual int_type overflow(int_type c);

  // Read one character
  virtual int_type underflow();

  virtual std::streampos seekpos(
    std::streampos sp,
    std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);

  virtual std::streamsize xsgetn(char* s, std::streamsize num);

  // write multiple characters
  virtual std::streamsize xsputn(const char* s, std::streamsize num);

private:
  gzFile m_gz_file;
  static const unsigned int kBufSize = 1024;
  char m_buf[kBufSize];
};

#endif  // MERT_GZFILEBUF_H_
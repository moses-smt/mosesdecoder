#ifndef MERT_GZFILEBUF_H_
#define MERT_GZFILEBUF_H_

#include <streambuf>
#include <zlib.h>
#include <cstring>

class GzFileBuf : public std::streambuf
{
public:
  explicit GzFileBuf(const char *filename) {
    m_gz_file = gzopen(filename, "rb");
    setg(m_buf + sizeof(int),     // beginning of putback area
         m_buf + sizeof(int),     // read position
         m_buf + sizeof(int));    // end position
  }

  virtual ~GzFileBuf() {
    gzclose(m_gz_file);
  }

protected:
  virtual int_type overflow(int_type c) {
    throw;
  }

  // write multiple characters
  virtual std::streamsize xsputn(const char* s,
                                 std::streamsize num) {
    throw;
  }

  virtual std::streampos seekpos(
      std::streampos sp,
      std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) {
    throw;
  }

  // read one character
  virtual int_type underflow() {
    // is read position before end of m_buf?
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    /* process size of putback area
     * - use number of characters read
     * - but at most four
     */
    unsigned int num_put_back = gptr() - eback();
    if (num_put_back > sizeof(int)) {
      num_put_back = sizeof(int);
    }

    /* copy up to four characters previously read into
     * the putback m_buf (area of first four characters)
     */
    std::memmove(m_buf + (sizeof(int) - num_put_back),
                 gptr() - num_put_back, num_put_back);

    // read new characters
    const int num = gzread(m_gz_file, m_buf + sizeof(int),
                           kBufSize - sizeof(int));
    if (num <= 0) {
      // ERROR or EOF
      return EOF;
    }

    // reset m_buf pointers
    setg(m_buf + (sizeof(int) - num_put_back),   // beginning of putback area
         m_buf + sizeof(int),                // read position
         m_buf + sizeof(int) + num);           // end of buffer

    // return next character
    return traits_type::to_int_type(*gptr());
  }

  std::streamsize xsgetn(char* s,
                         std::streamsize num) {
    return gzread(m_gz_file,s,num);
  }

private:
  gzFile m_gz_file;
  static const unsigned int kBufSize = 1024;
  char m_buf[kBufSize];
};

#endif  // MERT_GZFILEBUF_H_

#include "GzFileBuf.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>

GzFileBuf::GzFileBuf(const char* filename)
{
  m_gz_file = gzopen(filename, "rb");
  if (m_gz_file == NULL) {
    std::cerr << "ERROR: Failed to open " << filename << std::endl;
    std::exit(1);
  }
  setg(m_buf + sizeof(int),     // beginning of putback area
       m_buf + sizeof(int),     // read position
       m_buf + sizeof(int));    // end position
}

GzFileBuf::~GzFileBuf()
{
  gzclose(m_gz_file);
}

int GzFileBuf::overflow(int_type c)
{
  throw;
}

// read one character
int GzFileBuf::underflow()
{
  // is read position before end of m_buf?
  if (gptr() < egptr()) {
    return traits_type::to_int_type(*gptr());
  }

  /* process size of putback area
   * - use number of characters read
   * - but at most four
   */
  unsigned int num_put_back = static_cast<unsigned int>(gptr() - eback());
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
    return EOF;           // NOTE: the macro EOF defined in stdio.h
  }

  // reset m_buf pointers
  setg(m_buf + (sizeof(int) - num_put_back),   // beginning of putback area
       m_buf + sizeof(int),                // read position
       m_buf + sizeof(int) + num);           // end of buffer

  // return next character
  return traits_type::to_int_type(*gptr());
}

std::streampos GzFileBuf::seekpos(
  std::streampos sp,
  std::ios_base::openmode which)
{
  throw;
}

std::streamsize GzFileBuf::xsgetn(char* s,
                                  std::streamsize num)
{
  return static_cast<std::streamsize>(gzread(m_gz_file,s,num));
}

std::streamsize GzFileBuf::xsputn(const char* s,
                                  std::streamsize num)
{
  throw;
}

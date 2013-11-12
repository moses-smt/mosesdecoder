#ifndef moses_gzfile_buf_h
#define moses_gzfile_buf_h

#include <streambuf>
#include <zlib.h>
#include <cstring>

/** wrapper around gzip input stream. Unknown parentage
 *  @todo replace with boost version - output stream already uses it
 */
class gzfilebuf : public std::streambuf
{
public:
  gzfilebuf(const char *filename) {
    _gzf = gzopen(filename, "rb");
    setg (_buff+sizeof(int),     // beginning of putback area
          _buff+sizeof(int),     // read position
          _buff+sizeof(int));    // end position
  }
  ~gzfilebuf() {
    gzclose(_gzf);
  }
protected:
  virtual int_type overflow (int_type /* c */) {
    throw;
  }

  // write multiple characters
  virtual
  std::streamsize xsputn (const char* /* s */,
                          std::streamsize /* num */) {
    throw;
  }

  virtual std::streampos seekpos ( std::streampos /* sp */, std::ios_base::openmode /* which = std::ios_base::in | std::ios_base::out */ ) {
    throw;
  }

  //read one character
  virtual int_type underflow () {
    // is read position before end of _buff?
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    /* process size of putback area
     * - use number of characters read
     * - but at most four
     */
    unsigned int numPutback = gptr() - eback();
    if (numPutback > sizeof(int)) {
      numPutback = sizeof(int);
    }

    /* copy up to four characters previously read into
     * the putback _buff (area of first four characters)
     */
    std::memmove (_buff+(sizeof(int)-numPutback), gptr()-numPutback,
                  numPutback);

    // read new characters
    int num = gzread(_gzf, _buff+sizeof(int), _buffsize-sizeof(int));
    if (num <= 0) {
      // ERROR or EOF
      return EOF;
    }

    // reset _buff pointers
    setg (_buff+(sizeof(int)-numPutback),   // beginning of putback area
          _buff+sizeof(int),                // read position
          _buff+sizeof(int)+num);           // end of buffer

    // return next character
    return traits_type::to_int_type(*gptr());
  }

  std::streamsize xsgetn (char* s,
                          std::streamsize num) {
    return gzread(_gzf,s,num);
  }

private:
  gzFile _gzf;
  static const unsigned int _buffsize = 1024;
  char _buff[_buffsize];
};

#endif

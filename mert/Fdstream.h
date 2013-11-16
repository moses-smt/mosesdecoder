/*
 * This class creates c++ like stream from file descriptor
 * It uses gcc-specific functions, therefore is not portable
 *
 * Jeroen Vermeulen reckons that it can be replaced with Boost's io::stream_buffer
 *
 */


#ifndef _FDSTREAM_
#define _FDSTREAM_

#include <iostream>
#include <string>

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
#include <ext/stdio_filebuf.h>

#define BUFFER_SIZE (32768)

namespace MosesTuning
{

class _fdstream
{
protected:
  _fdstream() :
    _file_descriptor(-1), _filebuf(NULL) {
  }

  _fdstream(int file_descriptor, std::ios_base::openmode openmode) :
    _file_descriptor(file_descriptor), _openmode(openmode) {
    _filebuf = NULL;
    open(file_descriptor, openmode);
  }

  std::ios_base::openmode openmode() const {
    return _openmode;
  }

  void open(int file_descriptor, std::ios_base::openmode openmode) {
    if (!_filebuf)
      // We create a C++ stream from a file descriptor
      // stdio_filebuf is not synced with stdio.
      // From GCC 3.4.0 on exists in addition stdio_sync_filebuf
      // You can also create the filebuf from a FILE* with
      // FILE* f = fdopen(file_descriptor, mode);
      _filebuf = new __gnu_cxx::stdio_filebuf<char> (file_descriptor,
          openmode);
  }

  virtual ~_fdstream() {
    close(_file_descriptor);
    delete _filebuf;
    _filebuf = NULL;
  }

  int _file_descriptor;
  __gnu_cxx::stdio_filebuf<char>* _filebuf;
  std::ios_base::openmode _openmode;
};

class ifdstream : public _fdstream
{
public:
  ifdstream() :
    _fdstream(), _stream(NULL) {
  }

  ifdstream(int file_descriptor) :
    _fdstream(file_descriptor, std::ios_base::in) {
    _stream = new std::istream(_filebuf);
  }

  void open(int file_descriptor) {
    if (!_stream) {
      _fdstream::open(file_descriptor, std::ios_base::in);
      _stream = new std::istream(_filebuf);
    }
  }

  ifdstream& operator>> (std::string& str) {
    (*_stream) >> str;

    return *this;
  }

  std::size_t getline(std::string& str) {
    char tmp[BUFFER_SIZE];
    std::size_t ret = getline(tmp, BUFFER_SIZE);
    str = tmp;
    return ret;
  }

  std::size_t getline(char* s, std::streamsize n) {
    return (getline(s, n, '\n'));
  }

  std::size_t getline(char* s, std::streamsize n, char delim) {
    int i = 0;
    do {
      s[i] = _stream->get();
      i++;
    } while(i < n-1 && s[i-1] != delim && s[i-1] != '\0');

    s[i-1] = '\0'; // overwrite the delimiter given with string end

    return i-1;
  }

  ~ifdstream() {
    //this->~_fdstream();
    delete _stream;
  }

private:
  std::istream* _stream;
};

class ofdstream : public _fdstream
{
public:
  ofdstream() :
    _fdstream(), _stream(NULL) {
  }

  ofdstream(int file_descriptor) :
    _fdstream(file_descriptor, std::ios_base::out) {
    _stream = new std::ostream(_filebuf);
  }

  void open(int file_descriptor) {
    if (!_stream) {
      _fdstream::open(file_descriptor, std::ios_base::out);
      _stream = new std::ostream(_filebuf);
    }
  }


  ofdstream& operator<< (const std::string& str) {
    if (_stream->good())
      (*_stream) << str;

    _stream->flush();
    return *this;
  }

  ~ofdstream() {
    //this->~_fdstream();
    delete _stream;
  }

private:
  std::ostream* _stream;
};

#else
#error "Not supported"
#endif

}

#endif // _FDSTREAM_

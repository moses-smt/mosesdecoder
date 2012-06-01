#ifndef FILESTREAM_H_
#define FILESTREAM_H_

#include <fstream>
#include <streambuf>
#include <string>

class inputfilestream : public std::istream
{
protected:
  std::streambuf *m_streambuf;
  bool is_good;

public:
  explicit inputfilestream(const std::string &filePath);
  ~inputfilestream();
  bool good() const { return is_good; }
  void close();
};

class outputfilestream : public std::ostream
{
protected:
  std::streambuf *m_streambuf;
  bool is_good;

public:
  explicit outputfilestream(const std::string &filePath);
  ~outputfilestream();
  bool good() const { return is_good; }
  void close();
};

#endif // FILESTREAM_H_

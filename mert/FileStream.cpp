#include "FileStream.h"

#include <stdexcept>
#include "gzfilebuf.h"

using namespace std;

inputfilestream::inputfilestream(const std::string &filePath)
  : std::istream(0), m_streambuf(0)
{
  // check if file is readable
  std::filebuf* fb = new std::filebuf();
  is_good = (fb->open(filePath.c_str(), std::ios::in) != NULL);

  if (filePath.size() > 3 &&
      filePath.substr(filePath.size() - 3, 3) == ".gz") {
    fb->close();
    delete fb;
    m_streambuf = new gzfilebuf(filePath.c_str());
  } else {
    m_streambuf = fb;
  }
  this->init(m_streambuf);
}

inputfilestream::~inputfilestream()
{
  delete m_streambuf;
  m_streambuf = 0;
}

void inputfilestream::close()
{
}

outputfilestream::outputfilestream(const std::string &filePath)
  : std::ostream(0), m_streambuf(0)
{
  // check if file is readable
  std::filebuf* fb = new std::filebuf();
  is_good = (fb->open(filePath.c_str(), std::ios::out) != NULL);

  if (filePath.size() > 3 && filePath.substr(filePath.size() - 3, 3) == ".gz") {
    throw runtime_error("Output to a zipped file not supported!");
  } else {
    m_streambuf = fb;
  }
  this->init(m_streambuf);
}

outputfilestream::~outputfilestream()
{
  delete m_streambuf;
  m_streambuf = 0;
}

void outputfilestream::close()
{
}

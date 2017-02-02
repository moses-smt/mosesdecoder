#include "FileStream.h"

#include <stdexcept>
#include "GzFileBuf.h"

using namespace std;

namespace
{
bool IsGzipFile(const std::string &filename)
{
  return filename.size() > 3 &&
         filename.substr(filename.size() - 3, 3) == ".gz";
}
} // namespace

inputfilestream::inputfilestream(const std::string &filePath)
  : std::istream(0), m_streambuf(0), m_is_good(false)
{
  // check if file is readable
  std::filebuf* fb = new std::filebuf();
  m_is_good = (fb->open(filePath.c_str(), std::ios::in) != NULL);

  if (IsGzipFile(filePath)) {
    fb->close();
    delete fb;
    m_streambuf = new GzFileBuf(filePath.c_str());
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

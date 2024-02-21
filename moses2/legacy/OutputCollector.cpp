#include "OutputCollector.h"

namespace Moses2
{
  OutputCollector::OutputCollector(std::string xout, std::string xerr = "") :
  m_nextOutput(0) {
  // TO DO open magic streams instead of regular ofstreams! [UG]

  if (xout == "/dev/stderr") {
    m_outStream = &std::cerr;
    m_isHoldingOutputStream = false;
  }
  else if (xout.size() && xout != "/dev/stdout" && xout != "-") {
    m_outStream = new std::ofstream(xout.c_str());
    UTIL_THROW_IF2(!m_outStream->good(),
      "Failed to open output file" << xout);
    m_isHoldingOutputStream = true;
  }
  else {
    m_outStream = &std::cout;
    m_isHoldingOutputStream = false;
  }

  if (xerr == "/dev/stdout") {
    m_debugStream = &std::cout;
    m_isHoldingDebugStream = false;
  }
  else if (xerr.size() && xerr != "/dev/stderr") {
    m_debugStream = new std::ofstream(xerr.c_str());
    UTIL_THROW_IF2(!m_debugStream->good(),
      "Failed to open debug stream" << xerr);
    m_isHoldingDebugStream = true;
  }
  else {
    m_debugStream = &std::cerr;
    m_isHoldingDebugStream = false;
  }
}


}


#include "OutputCollector.h"

namespace Moses2
{
OutputCollector::OutputCollector(std::ostream* outStream,
  std::ostream* debugStream) :
  m_nextOutput(0), m_outStream(outStream), m_debugStream(debugStream), m_isHoldingOutputStream(
    false), m_isHoldingDebugStream(false) {
}

OutputCollector::OutputCollector(std::string xout, std::string xerr) :
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

OutputCollector::~OutputCollector() {
if (m_isHoldingOutputStream) delete m_outStream;
if (m_isHoldingDebugStream) delete m_debugStream;
}


void OutputCollector::Write(int sourceId, const std::string& output, const std::string& debug) {
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif
  if (sourceId == m_nextOutput) {
    //This is the one we were expecting
    *m_outStream << output << std::flush;
    *m_debugStream << debug << std::flush;
    ++m_nextOutput;
    //see if there's any more
    std::unordered_map<int, std::string>::iterator iter;
    while ((iter = m_outputs.find(m_nextOutput)) != m_outputs.end()) {
      *m_outStream << iter->second << std::flush;
      ++m_nextOutput;
      std::unordered_map<int, std::string>::iterator debugIter = m_debugs.find(
        iter->first);
      m_outputs.erase(iter);
      if (debugIter != m_debugs.end()) {
        *m_debugStream << debugIter->second << std::flush;
        m_debugs.erase(debugIter);
      }
    }
  }
  else {
    //save for later
    m_outputs[sourceId] = output;
    m_debugs[sourceId] = debug;
  }
}

}


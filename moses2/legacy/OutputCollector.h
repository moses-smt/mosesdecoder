/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2011 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#pragma once

#ifdef WITH_THREADS
#include <boost/thread/mutex.hpp>
#endif

#ifdef BOOST_HAS_PTHREADS
#include <pthread.h>
#endif

#include <iostream>
#include <unordered_map>
#include <ostream>
#include <fstream>
#include <string>
#include "util/exception.hh"

namespace Moses2
{
/**
 * Makes sure output goes in the correct order when multi-threading
 **/
class OutputCollector
{
public:
  OutputCollector(std::ostream* outStream = &std::cout,
    std::ostream* debugStream = &std::cerr);

  OutputCollector(std::string xout, std::string xerr = "");

  ~OutputCollector();

  void HoldOutputStream() {
    m_isHoldingOutputStream = true;
  }

  void HoldDebugStream() {
    m_isHoldingDebugStream = true;
  }

  bool OutputIsCout() const {
    return (m_outStream == &std::cout);
  }

  /**
   * Write or cache the output, as appropriate.
   **/
  void Write(int sourceId, const std::string& output, const std::string& debug =
    "");

private:
  std::unordered_map<int, std::string> m_outputs;
  std::unordered_map<int, std::string> m_debugs;
  int m_nextOutput;
  std::ostream* m_outStream;
  std::ostream* m_debugStream;
  bool m_isHoldingOutputStream;
  bool m_isHoldingDebugStream;
#ifdef WITH_THREADS
  boost::mutex m_mutex;
#endif

public:
  void SetOutputStream(std::ostream* outStream) {
    m_outStream = outStream;
  }

};

}  // namespace Moses


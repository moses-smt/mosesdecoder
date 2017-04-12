// $Id: OutputFileStream.cpp 2780 2010-01-29 17:11:17Z bojar $

/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2006 University of Edinburgh

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

#include <iostream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "OutputFileStream.h"
#include "gzfilebuf.h"

using namespace std;
using namespace boost::algorithm;

namespace probingpt
{
OutputFileStream::OutputFileStream() :
  boost::iostreams::filtering_ostream(), m_outFile(NULL), m_open(false)
{
}

OutputFileStream::OutputFileStream(const std::string &filePath) :
  m_outFile(NULL), m_open(false)
{
  Open(filePath);
}

OutputFileStream::~OutputFileStream()
{
  Close();
}

bool OutputFileStream::Open(const std::string &filePath)
{
  assert(!m_open);
  if (filePath == std::string("-")) {
    // Write to standard output.  Leave m_outFile null.
    this->push(std::cout);
  } else {
    m_outFile = new ofstream(filePath.c_str(),
                             ios_base::out | ios_base::binary);
    if (m_outFile->fail()) {
      return false;
    }

    if (ends_with(filePath, ".gz")) {
      this->push(boost::iostreams::gzip_compressor());
    }
    this->push(*m_outFile);
  }

  m_open = true;
  return true;
}

void OutputFileStream::Close()
{
  if (!m_open) return;
  this->flush();
  if (m_outFile) {
    this->pop(); // file

    m_outFile->close();
    delete m_outFile;
    m_outFile = NULL;
  }
  m_open = false;
}

}


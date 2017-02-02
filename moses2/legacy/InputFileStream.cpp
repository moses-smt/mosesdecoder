// $Id$

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

#include "InputFileStream.h"
#include "gzfilebuf.h"
#include <iostream>

using namespace std;

namespace Moses2
{

InputFileStream::InputFileStream(const std::string &filePath) :
  std::istream(NULL), m_streambuf(NULL)
{
  if (filePath.size() > 3 && filePath.substr(filePath.size() - 3, 3) == ".gz") {
    m_streambuf = new gzfilebuf(filePath.c_str());
  } else {
    std::filebuf* fb = new std::filebuf();
    fb = fb->open(filePath.c_str(), std::ios::in);
    if (!fb) {
      cerr << "Can't read " << filePath.c_str() << endl;
      exit(1);
    }
    m_streambuf = fb;
  }
  this->init(m_streambuf);
}

InputFileStream::~InputFileStream()
{
  delete m_streambuf;
  m_streambuf = NULL;
}

void InputFileStream::Close()
{
}

}


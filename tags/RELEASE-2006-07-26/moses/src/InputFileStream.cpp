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

#include <boost/iostreams/filter/gzip.hpp>

#if HAVE_BZIP2
#include <boost/iostreams/filter/bzip2.hpp>
#endif

#include "InputFileStream.h"

InputFileStream::InputFileStream(const std::string &filePath)
{
  using namespace boost::iostreams;

  if (filePath.size() > 3 &&
      filePath.substr(filePath.size() - 3, 3) == ".gz")
  {
    m_file.open(filePath.c_str(), ios_base::in | ios_base::binary);
    push(gzip_decompressor());
  }
#if HAVE_BZIP2
  else if (filePath.size() > 4 &&
      filePath.substr(filePath.size() - 4, 4) == ".bz2")
  {
    m_file.open(filePath.c_str(), ios_base::in | ios_base::binary);
    push(bzip2_decompressor());
  }
#endif
  else
  {
    m_file.open(filePath.c_str());
  }
  push(m_file);
}

void InputFileStream::Close()
{
  pop();
}


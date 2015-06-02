// $Id: InputFileStream.h 2939 2010-02-24 11:15:44Z jfouet $

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

#pragma once

#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <boost/iostreams/filtering_stream.hpp>

namespace Moses
{

/** Version of std::ostream with transparent compression.
 *
 * Transparently compresses output when writing to a file whose name ends in
 * ".gz".  Or, writes to stdout instead of a file when given a filename
 * consisting of just a dash ("-").
 */
class OutputFileStream : public boost::iostreams::filtering_ostream
{
private:
  /** File that needs flushing & closing when we close this stream.
   *
   * Is NULL when no file is opened, e.g. when writing to standard output.
   */
  std::ofstream *m_outFile;

  /// Is this stream open?
  bool m_open;

public:
  /** Create an unopened OutputFileStream.
   *
   * Until it's been opened, nothing can be done with this stream.
   */
  OutputFileStream();

  /// Create an OutputFileStream, and open it by calling Open().
  OutputFileStream(const std::string &filePath);
  virtual ~OutputFileStream();

  // TODO: Can we please just always throw an exception when this fails?
  /** Open stream.
   *
   * If filePath is "-" (just a dash), this opens the stream for writing to
   * standard output.  Otherwise, it opens the given file.  If the filename
   * has the ".gz" suffix, output will be transparently compressed.
   *
   * Call Close() to close the file.
   *
   * Returns whether opening the file was successful.  It may also throw an
   * exception on failure.
   */
  bool Open(const std::string &filePath);

  /// Flush and close stream.  After this, the stream can be opened again.
  void Close();
};

}


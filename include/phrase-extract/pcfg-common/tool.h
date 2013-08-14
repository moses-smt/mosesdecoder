/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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
#ifndef PCFG_TOOL_H_
#define PCFG_TOOL_H_

#include <boost/program_options/cmdline.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace Moses
{
namespace PCFG
{

class Tool
{
public:
  virtual ~Tool() {}

  const std::string &name() const {
    return name_;
  }

  virtual int Main(int argc, char *argv[]) = 0;

protected:
  Tool(const std::string &name) : name_(name) {}

  // Returns the boost::program_options style that should be used by all tools.
  static int CommonOptionStyle() {
    namespace cls = boost::program_options::command_line_style;
    return cls::default_style & (~cls::allow_guessing);
  }

  void Warn(const std::string &msg) const {
    std::cerr << name_ << ": warning: " << msg << std::endl;
  }

  void Error(const std::string &msg) const {
    std::cerr << name_ << ": error: " << msg << std::endl;
    std::exit(1);
  }

  // Initialises the tool's main input stream and returns a reference that is
  // valid for the remainder of the tool's lifetime.  If filename is empty or
  // "-" then input is standard input; otherwise it is the named file.  Calls
  // Error() if the file cannot be opened for reading.
  std::istream &OpenInputOrDie(const std::string &filename);

  // Initialises the tool's main output stream and returns a reference that is
  // valid for the remainder of the tool's lifetime.  If filename is empty or
  // "-" then output is standard output; otherwise it is the named file.  Calls
  // Error() if the file cannot be opened for writing.
  std::ostream &OpenOutputOrDie(const std::string &filename);

  // Opens the named input file using the supplied ifstream.  Calls Error() if
  // the file cannot be opened for reading.
  void OpenNamedInputOrDie(const std::string &, std::ifstream &);

  // Opens the named output file using the supplied ofstream.  Calls Error() if
  // the file cannot be opened for writing.
  void OpenNamedOutputOrDie(const std::string &, std::ofstream &);

private:
  std::string name_;
  std::istream *input_ptr_;
  std::ifstream input_file_stream_;
  std::ostream *output_ptr_;
  std::ofstream output_file_stream_;
};

}  // namespace PCFG
}  // namespace Moses

#endif

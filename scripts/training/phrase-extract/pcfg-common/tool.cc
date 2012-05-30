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

#include "tool.h"

#include <sstream>

namespace Moses {
namespace PCFG {

std::istream &Tool::OpenInputOrDie(const std::string &filename) {
  // TODO Check that function is only called once?
  if (filename.empty() || filename == "-") {
    input_ptr_ = &(std::cin);
  } else {
    input_file_stream_.open(filename.c_str());
    if (!input_file_stream_) {
      std::ostringstream msg;
      msg << "failed to open input file: " << filename;
      Error(msg.str());
    }
    input_ptr_ = &input_file_stream_;
  }
  return *input_ptr_;
}

std::ostream &Tool::OpenOutputOrDie(const std::string &filename) {
  // TODO Check that function is only called once?
  if (filename.empty() || filename == "-") {
    output_ptr_ = &(std::cout);
  } else {
    output_file_stream_.open(filename.c_str());
    if (!output_file_stream_) {
      std::ostringstream msg;
      msg << "failed to open output file: " << filename;
      Error(msg.str());
    }
    output_ptr_ = &output_file_stream_;
  }
  return *output_ptr_;
}

void Tool::OpenNamedInputOrDie(const std::string &filename,
                               std::ifstream &stream) {
  stream.open(filename.c_str());
  if (!stream) {
    std::ostringstream msg;
    msg << "failed to open input file: " << filename;
    Error(msg.str());
  }
}

void Tool::OpenNamedOutputOrDie(const std::string &filename,
                                std::ofstream &stream) {
  stream.open(filename.c_str());
  if (!stream) {
    std::ostringstream msg;
    msg << "failed to open output file: " << filename;
    Error(msg.str());
  }
}

}  // namespace PCFG
}  // namespace Moses

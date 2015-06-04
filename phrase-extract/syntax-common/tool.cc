#include "tool.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <boost/program_options/cmdline.hpp>

namespace MosesTraining {
namespace Syntax {

int Tool::MosesOptionStyle() {
  namespace cls = boost::program_options::command_line_style;
  return cls::allow_long | cls::long_allow_adjacent | cls::long_allow_next;
}

void Tool::Warn(const std::string &msg) const {
  std::cerr << name_ << ": warning: " << msg << std::endl;
}

void Tool::Error(const std::string &msg) const {
  std::cerr << name_ << ": error: " << msg << std::endl;
  std::exit(1);
}

void Tool::OpenInputFileOrDie(const std::string &filename,
                              std::ifstream &stream) {
  stream.open(filename.c_str());
  if (!stream) {
    std::ostringstream msg;
    msg << "failed to open input file: " << filename;
    Error(msg.str());
  }
}

void Tool::OpenOutputFileOrDie(const std::string &filename,
                               std::ofstream &stream) {
  stream.open(filename.c_str());
  if (!stream) {
    std::ostringstream msg;
    msg << "failed to open output file: " << filename;
    Error(msg.str());
  }
}

void Tool::OpenOutputFileOrDie(const std::string &filename,
                               Moses::OutputFileStream &stream) {
  bool ret = stream.Open(filename);
  if (!ret) {
    std::ostringstream msg;
    msg << "failed to open output file: " << filename;
    Error(msg.str());
  }
}

}  // namespace Syntax
}  // namespace MosesTraining

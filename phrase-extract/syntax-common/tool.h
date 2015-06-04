#pragma once

#include <fstream>
#include <string>

#include "OutputFileStream.h"

namespace MosesTraining {
namespace Syntax {

/*! Base class for command-line based tools.
 */
class Tool {
 public:
  virtual ~Tool() {}

  //! Get the name of the tool.
  const std::string &name() const { return name_; }

  //! Virtual main function to be provided by subclass.
  virtual int Main(int argc, char *argv[]) = 0;

 protected:
  Tool(const std::string &name) : name_(name) {}

  //! Returns a boost::program_options style that is consistent with other
  //! Moses tools (extract-rules, score, etc.).
  static int MosesOptionStyle();

  //! Write a formatted warning message to standard error.
  void Warn(const std::string &) const;

  //! Write a formatted error message to standard error and call exit(1).
  void Error(const std::string &msg) const;

  //! Opens the named input file using the supplied ifstream.  Calls Error() if
  //! the file cannot be opened for reading.
  void OpenInputFileOrDie(const std::string &, std::ifstream &);

  //! Opens the named output file using the supplied ofstream.  Calls Error() if
  //! the file cannot be opened for writing.
  void OpenOutputFileOrDie(const std::string &, std::ofstream &);

  //! Opens the named output file using the supplied OutputFileStream.  Calls
  //! Error() if the file cannot be opened for writing.
  void OpenOutputFileOrDie(const std::string &, Moses::OutputFileStream &);

 private:
  std::string name_;
};

}  // namespace Syntax
}  // namespace MosesTraining

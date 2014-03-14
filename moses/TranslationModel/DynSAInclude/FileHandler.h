#ifndef moses_DynSAInclude_file_h
#define moses_DynSAInclude_file_h

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <string>
#include "util/exception.hh"
#include "fdstream.h"
#include "utils.h"

namespace Moses
{
typedef std::string FileExtension;

//! @todo ask abby2
class FileHandler: public std::fstream
{
public:
  // descriptors for stdin and stdout
  static const std::string kStdInDescriptor;	// file name for std::cin
  static const std::string kStdOutDescriptor;	// file name for std::cout
  // compression commands
  static const std::string kCatCommand;	// i.e. no compression
  static const std::string kGzipCommand;	// gzip -f
  static const std::string kGunzipCommand;	// gunzip -f
  static const std::string kBzip2Command;	// bzip2 -f
  static const std::string kBunzip2Command;	// bunzip2 -f

  // open file or wrap stdin or stdout
  FileHandler(const std::string & path,
              std::ios_base::openmode flags = std::ios::in,
              bool checkExists = true);
  ~FileHandler();
  // file utilities
  static bool getCompressionCmds(const std::string & filepath,
                                 std::string & compressionCmd,
                                 std::string & decompressionCmd,
                                 std::string & compressionSuffix);

  // data accessors
  std::string getPath() {
    return path_;
  }
  std::ios_base::openmode getFlags() {
    return flags_;
  }
  bool isStdIn() {
    return path_ == FileHandler::kStdInDescriptor;
  }
  bool isStdOut() {
    return path_ == FileHandler::kStdOutDescriptor;
  }
  bool reset();
protected:
  static const FileExtension kGzipped;
  static const FileExtension kBzipped2;
  bool fileExists();
  bool setStreamBuffer(bool checkExists);
  bool isCompressedFile(std::string & cmd);
  fdstreambuf* openCompressedFile(const char* cmd);
  std::string path_; // file path
  std::ios_base::openmode flags_;	// open flags
  std::streambuf* buffer_;	// buffer to either gzipped or standard data
  std::FILE* fp_;	//file pointer to handle pipe data
};

} // end namespace

#endif

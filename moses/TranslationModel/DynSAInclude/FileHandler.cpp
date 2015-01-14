#include "FileHandler.h"
#include <stdio.h>

#ifdef WIN32
#define popen(A, B) _popen(A, B)
#define pclose(A) _pclose(A)
#endif

namespace Moses
{

// FileHandler class
const std::string FileHandler::kStdInDescriptor = "___stdin___";
const std::string FileHandler::kStdOutDescriptor = "___stdout___";
// compression commands
const FileExtension FileHandler::kGzipped = ".gz";
const FileExtension FileHandler::kBzipped2 = ".bz2";

const std::string FileHandler::kCatCommand = "cat";
const std::string FileHandler::kGzipCommand = "gzip -f";
const std::string FileHandler::kGunzipCommand = "gunzip -f";
const std::string FileHandler::kBzip2Command = "bzip2 -f";
const std::string FileHandler::kBunzip2Command = "bunzip2 -f";

FileHandler::FileHandler(const std::string & path, std::ios_base::openmode flags, bool /* checkExists */)
  : std::fstream((const char*) NULL), path_(path), flags_(flags), buffer_(NULL), fp_(NULL)
{
  if( !(flags^(std::ios::in|std::ios::out)) ) {
    fprintf(stderr, "ERROR: FileHandler does not support bidirectional files (%s).\n", path_.c_str());
    exit(EXIT_FAILURE);
  } else {
    bool ret = setStreamBuffer(flags & std::ios::in);
    UTIL_THROW_IF2(!ret, "Unable to set stream buffer");
  }
  this->precision(32);
}

FileHandler::~FileHandler()
{
#ifndef NO_PIPES
  if( fp_ != 0 )
    pclose(fp_);
#endif
  if( path_ != FileHandler::kStdInDescriptor &&
      path_ != FileHandler::kStdOutDescriptor )
    delete buffer_;
  if( this->is_open() )
    this->close();
}

fdstreambuf * FileHandler::openCompressedFile(const char * cmd)
{
  //bool isInput = (flags_ & std::ios::in);
  //open pipe to file with compression/decompression command
  const char * p_type = (flags_ & std::ios::in ? "r" : "w");
#ifndef NO_PIPES
  fp_ = popen(cmd, p_type);
#else
  fp_ = NULL;
#endif
  if( fp_ == NULL ) {
    //fprintf(stderr, "ERROR:Failed to open compressed file at %s\n", path_.c_str());
    perror("openCompressedFile: ");
    exit(EXIT_FAILURE);
  }
  //open streambuf with file descriptor
  return new fdstreambuf(fileno(fp_));
}

bool FileHandler::setStreamBuffer(bool checkExists)
{
  // redirect stdin or stdout if necesary
  if (path_ == FileHandler::kStdInDescriptor) {
    UTIL_THROW_IF2((flags_ & std::ios::in) == 0,
                   "Incorrect flags: " << flags_);
    std::streambuf* sb = std::cin.rdbuf();
    buffer_ = sb;
  } else if (path_ == FileHandler::kStdOutDescriptor) {
    UTIL_THROW_IF2((flags_ & std::ios::out) == 0,
                   "Incorrect flags: " << flags_);
    std::streambuf* sb = std::cout.rdbuf();
    buffer_ = sb;
  } else {
    // real file
    if( checkExists && ! fileExists() ) {
      fprintf(stderr, "ERROR: Failed to find file at %s\n", path_.c_str());
      exit(EXIT_FAILURE);
    }
    std::string cmd = "";
    if( isCompressedFile(cmd) && (! cmd.empty()) ) {
      buffer_ = openCompressedFile(cmd.c_str());
    } else {
      // open underlying filebuf
      std::filebuf* fb = new std::filebuf();
      fb->open(path_.c_str(), flags_);
      buffer_ = fb;
    }
  }
  if (!buffer_) {
    fprintf(stderr, "ERROR:Failed to open file at %s\n", path_.c_str());
    exit(EXIT_FAILURE);
  }
  this->init(buffer_);
  return true;
}

/*
 * Checks for compression via file extension. Currently checks for
 * ".gz" and ".bz2".
 */
bool FileHandler::isCompressedFile(std::string & cmd)
{
  bool compressed = false, isInput = (flags_ & std::ios::in);
  cmd = "";
  unsigned int len = path_.size();
  if( len > kGzipped.size()
      && path_.find(kGzipped) == len - kGzipped.size()) {
    //gzip file command to compress or decompress
    compressed = true;
    //			cmd = (isInput ? "exec gunzip -cf " : "exec gzip -c > ") + path_;
    cmd = (isInput ? "exec " + kGunzipCommand + "c "
           : "exec " + kGzipCommand + "c > ") + path_;
  } else if( len > kBzipped2.size() &&
             path_.find(kBzipped2) == len - kBzipped2.size()) {
    //do bzipped2 file command
    compressed = true;
    cmd = (isInput ? "exec " + kBunzip2Command + "c "
           : "exec " + kBzip2Command + "c > ") + path_;
  }
  return compressed;
}

bool FileHandler::fileExists()
{
  bool exists = false;
  struct stat f_info;
  if( stat(path_.c_str(), &f_info) == 0 ) //if stat() returns no errors
    exists = true;
  return( exists );
}

// static method used during preprocessing compressed files without
// opening fstream objects.
bool FileHandler::getCompressionCmds(const std::string & filepath, std::string & compressionCmd,
                                     std::string & decompressionCmd,
                                     std::string & compressionSuffix)
{
  // determine what compression and decompression cmds are suitable from filepath
  compressionCmd = kCatCommand;
  decompressionCmd = kCatCommand;
  if (filepath.length() > kGzipped.size() &&
      filepath.find(kGzipped) == filepath.length()
      - kGzipped.length()) {
    compressionCmd = kGzipCommand;
    decompressionCmd = kGunzipCommand;
    compressionSuffix = kGzipped;
  } else if (filepath.length() > kBzipped2.size() &&
             filepath.find(kBzipped2) == filepath.length()
             - kBzipped2.length() ) {
    compressionCmd = kBzip2Command;
    decompressionCmd = kBunzip2Command;
    compressionSuffix = kBzipped2;;
  }
  return (compressionCmd != kCatCommand && decompressionCmd != kCatCommand);
}

bool FileHandler::reset()
{
#ifndef NO_PIPES
  // move to beginning of file
  if (fp_ != 0) {
    //can't seek on a pipe so reopen
    pclose(fp_);
    std::string cmd = "";
    if (isCompressedFile(cmd) && ! cmd.empty())
      buffer_ = openCompressedFile(cmd.c_str());
    //reinitialize
    this->init(buffer_);
  } else
#endif
    buffer_->pubseekoff(0, std::ios_base::beg); //sets both get and put pointers to beginning of stream
  return true;
}
} //end namespace

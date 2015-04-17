#ifndef UTIL_TEMPFILE_H
#define UTIL_TEMPFILE_H

// Utilities for creating temporary files and directories.

#include <cstdio>
#include <cstdlib>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

#include "util/exception.hh"

namespace util
{

/** Temporary directory.
 *
 * Automatically creates, and on destruction deletes, a temporary directory.
 * The actual directory in the filesystem will only exist while the temp_dir
 * object exists.
 *
 * If the directory no longer exists by the time the temp_dir is destroyed,
 * no cleanup happens.
 */
class temp_dir : boost::noncopyable
{
public:
  temp_dir()
  {
    char buf[] = "tmpdir.XXXXXX";
    m_path = std::string(mkdtemp(buf));
  }

  ~temp_dir()
  {
    boost::filesystem::remove_all(path());
  }

  /// Return the temporary directory's full path.
  const std::string &path() const { return m_path; }

private:
  std::string m_path;
};


/** Temporary file.
 *
 * Automatically creates, and on destruction deletes, a temporary file.
 */
class temp_file : boost::noncopyable
{
public:
  temp_file()
  {
    char buf[] = "tmp.XXXXXX";
    const int fd = mkstemp(buf);
    if (fd == -1) throw ErrnoException();
    close(fd);
    m_path = buf;
  }

  ~temp_file()
  {
    boost::filesystem::remove(path());
  }

  /// Return the temporary file's full path.
  const std::string &path() const { return m_path; }

private:
  std::string m_path;
};

} // namespace util

#endif

#ifndef UTIL_TEMPFILE_H
#define UTIL_TEMPFILE_H

// Utilities for creating temporary files and directories.

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

#include "util/exception.hh"

namespace util
{

/// Obtain a directory for temporary files, e.g. /tmp.
std::string temp_location()
{
#if defined(_WIN32) || defined(_WIN64)
  char dir_buffer[1000];
  if (GetTempPathA(1000, dir_buffer) == 0)
    throw std::runtime_error("Could not read temporary directory.");
  return std::string(dir_buffer);
#else
  // POSIX says to try these environment variables, in this order:
  const char *const vars[] = {"TMPDIR", "TMP", "TEMPDIR", "TEMP", 0};
  for (int i=0; vars[i]; ++i)
  {
    const char *val = getenv(vars[i]);
    // Environment variable is set and nonempty.  Use it.
    if (val && *val) return val;
  }
  // No environment variables set.  Default to /tmp.
  return "/tmp";
#endif
}


#if defined(_WIN32) || defined(_WIN64)
/// Windows helper: create temporary filename.
std::string windows_tmpnam()
{
  const std::string tmp = temp_location();
  char output_buffer[MAX_PATH];
  if (GetTempFileNameA(tmp.c_str(), "tmp", 0, output_buffer) == 0)
    throw std::runtime_error("Could not create temporary file name.");
  return output_buffer;
}
#else
/** POSIX helper: create template for temporary filename.
 *
 * Writes the template into buf, which must have room for at least PATH_MAX
 * bytes.  The function fails if the template is too long.
 */
void posix_tmp_template(char *buf)
{
    const std::string tmp = temp_location();
    const std::string name_template = tmp + "/tmp.XXXXXX";
    if (name_template.size() >= PATH_MAX-1)
      throw std::runtime_error("Path for temp files is too long: " + tmp);
    strcpy(buf, name_template.c_str());
}
#endif


/** Temporary directory.
 *
 * Automatically creates, and on destruction deletes, a temporary directory.
 * The actual directory in the filesystem will only exist while the temp_dir
 * object exists.
 *
 * If the directory no longer exists by the time the temp_dir is destroyed,
 * cleanup is skipped.
 */
class temp_dir : boost::noncopyable
{
public:
  temp_dir()
  {
#if defined(_WIN32) || defined(_WIN64)
    m_path = windows_tmpnam();
    boost::filesystem::create_directory(m_path);
#else
    char buf[PATH_MAX];
    posix_tmp_template(buf);
    m_path = std::string(mkdtemp(buf));
#endif
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
 *
 * If the file no longer exists by the time the temp_file is destroyed,
 * cleanup is skipped.
 */
class temp_file : boost::noncopyable
{
public:
  temp_file()
  {
#if defined(_WIN32) || defined(_WIN64)
    m_path = windows_tmpnam();
    std::ofstream out(m_path.c_str());
    out.flush();
#else
    char buf[PATH_MAX];
    posix_tmp_template(buf);
    const int fd = mkstemp(buf);
    if (fd == -1) throw ErrnoException();
    close(fd);
    m_path = buf;
#endif
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

#include "util/file.hh"

#include "util/exception.hh"

#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

namespace util {

scoped_fd::~scoped_fd() {
  if (fd_ != kBadFD && close(fd_)) {
    std::cerr << "Could not close file " << fd_ << std::endl;
    std::abort();
  }
}

scoped_FILE::~scoped_FILE() {
  if (file_ && std::fclose(file_)) {
    std::cerr << "Could not close file " << std::endl;
    std::abort();
  }
}

FD OpenReadOrThrow(const char *name) {
  FD ret;
#ifdef WIN32

#else
  UTIL_THROW_IF(-1 == (ret = open(name, O_RDONLY)), ErrnoException, "while opening " << name);
#endif
  return ret;
}

FD CreateOrThrow(const char *name) {
  FD ret;
#ifdef WIN32

#else
  UTIL_THROW_IF(-1 == (ret = open(name, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR)), ErrnoException, "while creating " << name);
#endif
  return ret;
}

off_t SizeFile(FD fd) {
#ifdef WIN32
  return 0; // TODO WIN32

#else
  struct stat sb;
  if (fstat(fd, &sb) == -1 || (!sb.st_size && !S_ISREG(sb.st_mode))) return kBadSize;
  return sb.st_size;
#endif
}

void ReadOrThrow(FD fd, void *to_void, std::size_t amount) {
  uint8_t *to = static_cast<uint8_t*>(to_void);
  while (amount) {

#ifdef WIN32
	ssize_t ret; // TODO WIN32
#else
    ssize_t ret = read(fd, to, amount);
#endif
    if (ret == -1) UTIL_THROW(ErrnoException, "Reading " << amount << " from fd " << fd << " failed.");
    if (ret == 0) UTIL_THROW(Exception, "Hit EOF in fd " << fd << " but there should be " << amount << " more bytes to read.");
    amount -= ret;
    to += ret;
  }
}

void WriteOrThrow(FD fd, const void *data_void, std::size_t size) {
  const uint8_t *data = static_cast<const uint8_t*>(data_void);
  while (size) {
#ifdef WIN32
	ssize_t ret; // TODO WIN32
#else
    ssize_t ret = write(fd, data, size);
#endif
    if (ret < 1) UTIL_THROW(util::ErrnoException, "Write failed");
    data += ret;
    size -= ret;
  }
}

void RemoveOrThrow(const char *name) {
  UTIL_THROW_IF(std::remove(name), util::ErrnoException, "Could not remove " << name);
}

} // namespace util

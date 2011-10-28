#ifndef UTIL_FILE__
#define UTIL_FILE__

#include <cstdio>
#include "util/portability.hh"

namespace util {

class scoped_fd {
  public:
    scoped_fd() : fd_(kBadFD) {}

    explicit scoped_fd(FD fd) : fd_(fd) {}

    ~scoped_fd();

    void reset(FD to) {
      scoped_fd other(fd_);
      fd_ = to;
    }

    FD get() const { return fd_; }

    FD operator*() const { return fd_; }

    FD release() {
      FD ret = fd_;
      fd_ = kBadFD;
      return ret;
    }

    operator bool() { return fd_ != kBadFD; }

  private:
    FD fd_;

    scoped_fd(const scoped_fd &);
    scoped_fd &operator=(const scoped_fd &);
};

class scoped_FILE {
  public:
    explicit scoped_FILE(std::FILE *file = NULL) : file_(file) {}

    ~scoped_FILE();

    std::FILE *get() { return file_; }
    const std::FILE *get() const { return file_; }

    void reset(std::FILE *to = NULL) {
      scoped_FILE other(file_);
      file_ = to;
    }

  private:
    std::FILE *file_;
};

FD OpenReadOrThrow(const char *name);

FD CreateOrThrow(const char *name);

// Return value for SizeFile when it can't size properly.  
const off_t kBadSize = -1;
off_t SizeFile(FD fd);

void ReadOrThrow(FD fd, void *to, std::size_t size);
void WriteOrThrow(FD fd, const void *data_void, std::size_t size);

void RemoveOrThrow(const char *name);

} // namespace util

#endif // UTIL_FILE__

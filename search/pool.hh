// Very simple pool.  It can only allocate memory.  And all of the memory it
// allocates must be freed at the same time.  

#ifndef SEARCH_POOL__
#define SEARCH_POOL__

#include <boost/noncopyable.hpp>

#include <vector>

#include <stdint.h>

namespace search {

class Pool : boost::noncopyable {
  public:
    Pool();

    ~Pool();

    void *Allocate(size_t size) {
      void *ret = current_;
      current_ += size;
      if (current_ < current_end_) {
        return ret;
      } else {
        return More(size);
      }
    }

    void FreeAll();

  private:
    void *More(size_t size);

    std::vector<void *> free_list_;

    uint8_t *current_, *current_end_;
}; 

} // namespace lm

#endif // SEARCH_POOL__

#pragma once
#include <boost/thread.hpp>

namespace Moses
{
  class ThreadSafeCounter
  {
    size_t ctr;
    boost::mutex lock;
  public:
    ThreadSafeCounter();
    size_t operator++();
    size_t operator++(int);
    size_t operator--();
    size_t operator--(int);
    operator size_t() const;
  };

}



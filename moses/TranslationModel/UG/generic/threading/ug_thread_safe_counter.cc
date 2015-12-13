// #include "moses/TranslationModel/UG/generic/threading/ug_thread_safe_counter.h"
#include "ug_thread_safe_counter.h"
// obsolete once <atomic> can be assumed to be available everywhere 

namespace Moses
{
  ThreadSafeCounter::
  ThreadSafeCounter()
    : ctr(0)
  { }

  size_t
  ThreadSafeCounter::
  operator++()
  {
    boost::lock_guard<boost::mutex> guard(this->lock);
    return ++ctr;
  }

  size_t
  ThreadSafeCounter::
  operator++(int foo)
  {
    boost::lock_guard<boost::mutex> guard(this->lock);
    return ctr++;
  }

  ThreadSafeCounter::
  operator size_t() const
  {
    return ctr;
  }

  size_t
  ThreadSafeCounter::
  operator--()
  {
    boost::lock_guard<boost::mutex> guard(this->lock);
    return --ctr;
  }

  size_t
  ThreadSafeCounter::
  operator--(int foo)
  {
    boost::lock_guard<boost::mutex> guard(this->lock);
    return ctr--;
  }


}

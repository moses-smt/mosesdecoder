#include "util/random.hh"

#include <cstdlib>

#include <boost/thread/locks.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

namespace util
{
namespace
{
/** Lock to protect randomizer.
 *
 * This module is implemented in terms of rand()/srand() from <cstdlib>.
 * These functions are standard C, but they're not thread-safe.  Scalability
 * is not worth much complexity here, so just slap a mutex around it.
 */
boost::mutex rand_lock;
} // namespace

void rand_int_init(unsigned int seed)
{
  boost::lock_guard<boost::mutex> lock(rand_lock);
  srand(seed);
}


void rand_int_init()
{
  rand_int_init(time(NULL));
}

int rand_int()
{
  boost::lock_guard<boost::mutex> lock(rand_lock);
  return rand();
}
} // namespace util

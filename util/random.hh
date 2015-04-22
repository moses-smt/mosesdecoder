#ifndef UTIL_RANDOM_H
#define UTIL_RANDOM_H

namespace util
{

/** Initialize randomizer with a fixed seed.
 *
 * After this, unless the randomizer gets seeded again, consecutive calls to
 * rand_int() will return a sequence of pseudo-random numbers determined by
 * the seed.  Every time the randomizer is seeded with this same seed, it will
 * again start returning the same sequence of numbers.
 */
void rand_int_init(unsigned int);

/** Initialize randomizer based on current time.
 *
 * Call this to make the randomizer return hard-to-predict numbers.  It won't
 * produce high-grade randomness, but enough to make the program act
 * differently on different runs.
 */
void rand_int_init();

/** Return a pseudorandom number between 0 and RAND_MAX inclusive.
 *
 * Initialize (seed) the randomizer before starting to call this.
 */
int rand_int();

} // namespace util

#endif

#ifndef UTIL_RANDOM_H
#define UTIL_RANDOM_H

#include <cstdlib>
#include <limits>

namespace util
{
/** Thread-safe, cross-platform random number generator.
 *
 * This is not for proper security-grade randomness, but should be "good
 * enough" for producing arbitrary values of various numeric types.
 *
 * Before starting, call rand_init() to seed the randomizer.  There is no need
 * to do this more than once; in fact doing it more often is likely to make the
 * randomizer less effective.  Once that is done, call the rand(), rand_excl(),
 * and rand_incl() functions as needed to generate pseudo-random numbers.
 *
 * Probability distribution is roughly uniform, but for integral types is
 * skewed slightly towards lower numbers depending on how close "top" comes to
 * RAND_MAX.
 *
 * For floating-point types, resolution is limited; there will actually be
 * only RAND_MAX different possible values.
 */

/** Initialize randomizer with a fixed seed.
 *
 * After this, unless the randomizer gets seeded again, consecutive calls to
 * the random functions will return a sequence of pseudo-random numbers
 * determined by the seed.  Every time the randomizer is seeded with this same
 * seed, it will again start returning the same sequence of numbers.
 */
void rand_init(unsigned int);

/** Initialize randomizer based on current time.
 *
 * Call this to make the randomizer return hard-to-predict numbers.  It won't
 * produce high-grade randomness, but enough to make the program act
 * differently on different runs.
 *
 * The seed will be based on the current time in seconds.  So calling it twice
 * within the same second will just reset the randomizer to where it was before.
 * Don't do that.
 */
void rand_init();


/** Return a pseudorandom number between 0 and RAND_MAX inclusive.
 *
 * Initialize (seed) the randomizer before starting to call this.
 */
template<typename T> inline T rand();


/** Return a pseudorandom number in the half-open interval [bottom, top).
 *
 * Generates a value between "bottom" (inclusive) and "top" (exclusive),
 * assuming that (top - bottom) <= RAND_MAX.
 */
template<typename T> inline T rand_excl(T bottom, T top);


/** Return a pseudorandom number in the half-open interval [0, top).
 *
 * Generates a value between 0 (inclusive) and "top" (exclusive), assuming that
 * bottom <= RAND_MAX.
 */
template<typename T> inline T rand_excl(T top);


/** Return a pseudorandom number in the open interval [bottom, top].
 *
 * Generates a value between "bottom" and "top" inclusive, assuming that
 * (top - bottom) < RAND_MAX.
 */
template<typename T> inline T rand_incl(T bottom, T top);


/** Return a pseudorandom number in the open interval [0, top].
 *
 * Generates a value between 0 and "top" inclusive, assuming that
 * bottom < RAND_MAX.
 */
template<typename T> inline T rand_incl(T top);


/** Return a pseudorandom number which may be larger than RAND_MAX.
 *
 * The requested type must be integral, and its size must be an even multiple
 * of the size of an int.  The return value will combine one or more random
 * ints into a single value, which could get quite large.
 *
 * The result is nonnegative.  Because the constituent ints are also
 * nonnegative, the most significant bit in each of the ints will be zero,
 * so for a wider type, there will be "gaps" in the range of possible outputs.
 */
template<typename T> inline T wide_rand();

/** Return a pseudorandom number in [0, top), not limited to RAND_MAX.
 *
 * Works like wide_rand(), but if the requested type is wider than an int, it
 * accommodates larger top values than an int can represent.
 */
template<typename T> inline T wide_rand_excl(T top);

/** Return a pseudorandom number in [bottom, top), not limited to RAND_MAX.
 *
 * Works like wide_rand(), but if the requested type is wider than an int, it
 * accommodates larger value ranges than an int can represent.
 */
template<typename T> inline T wide_rand_excl(T bottom, T top);

/** Return a pseudorandom number in [0, top], not limited to RAND_MAX.
 *
 * Works like wide_rand(), but if the requested type is wider than an int, it
 * accommodates larger top values than an int can represent.
 */
template<typename T> inline T wide_rand_incl(T top);

/** Return a pseudorandom number in [bottom, top], not limited to RAND_MAX.
 *
 * Works like wide_rand(), but if the requested type is wider than an int, it
 * accommodates larger top values than an int can represent.
 */
template<typename T> inline T wide_rand_incl(T bottom, T top);


/// Implementation detail.  For the random module's internal use only.
namespace internal
{
/// The central call to the randomizer upon which this whole module is built.
int rand_int();

/// Helper template: customize random values to required ranges.
template<typename T, bool is_integer_type> struct random_scaler;

/// Specialized random_scaler for integral types.
template<typename T> struct random_scaler<T, true>
{
  static T rnd_excl(T value, T range) { return value % range; }
  static T rnd_incl(T value, T range) { return value % (range + 1); }
};

/// Specialized random_scaler for non-integral types.
template<typename T> struct random_scaler<T, false>
{
  static T rnd_excl(T value, T range)
  {
    // Promote RAND_MAX to T before adding one to avoid overflow.
    return range * value / (T(RAND_MAX) + 1);
  }
  static T rnd_incl(T value, T range) { return range * value / RAND_MAX; }
};

/// Helper for filling a wider variable with random ints.
template<typename T, size_t remaining_ints> struct wide_random_collector
{
  static T generate()
  {
    T one_int = util::rand<T>() << (8 * sizeof(int));
    return one_int | wide_random_collector<T, remaining_ints-1>::generate();
  }
};
/// Specialized wide_random_collector for generating just a single int.
template<typename T> struct wide_random_collector<T, 1>
{
  static T generate() { return util::rand<T>(); }
};

} // namespace internal


template<typename T> inline T rand()
{
  return T(util::internal::rand_int());
}

template<typename T> inline T rand_excl(T top)
{
  typedef internal::random_scaler<T, std::numeric_limits<T>::is_integer> scaler;
  return scaler::rnd_excl(util::rand<T>(), top);
}

template<typename T> inline T rand_excl(T bottom, T top)
{
  return bottom + rand_excl(top - bottom);
}

template<typename T> inline T rand_incl(T top)
{
  typedef internal::random_scaler<T, std::numeric_limits<T>::is_integer> scaler;
  return scaler::rnd_incl(util::rand<T>(), top);
}

template<typename T> inline T rand_incl(T bottom, T top)
{
  return bottom + rand_incl(top - bottom);
}

template<typename T> inline T wide_rand()
{
  return internal::wide_random_collector<T, sizeof(T)/sizeof(int)>::generate();
}

template<typename T> inline T wide_rand_excl(T top)
{
  typedef internal::random_scaler<T, std::numeric_limits<T>::is_integer> scaler;
  return scaler::rnd_excl(util::wide_rand<T>(), top);
}

template<typename T> inline T wide_rand_excl(T bottom, T top)
{
  return bottom + wide_rand_excl(top - bottom);
}

template<typename T> inline T wide_rand_incl(T top)
{
  typedef internal::random_scaler<T, std::numeric_limits<T>::is_integer> scaler;
  return scaler::rnd_incl(util::wide_rand<T>(), top);
}

template<typename T> inline T wide_rand_incl(T bottom, T top)
{
  return bottom + wide_rand_incl(top - bottom);
}
} // namespace util

#endif

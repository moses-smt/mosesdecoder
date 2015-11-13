#ifndef __sampling_h
#define __sampling_h
#include <boost/dynamic_bitset.hpp>
#include <vector>

#include "util/random.hh"

// Utility functions for proper sub-sampling.
// (c) 2007-2012 Ulrich Germann


namespace Moses
{

// select a random sample of size /s/ without restitution from the
// range of integers [0,N);
template<typename idx_t>
void
randomSample(std::vector<idx_t>& v, size_t s, size_t N)
{
  // see also Knuth: Art of Computer Programming Vol. 2, p. 142

  s = std::min(s,N);
  v.resize(s);

  // the first option tries to be a bit more efficient than O(N) in
  // picking the samples. The threshold is an ad-hoc, off-the-cuff
  // guess. I still need to figure out the optimal break-even point
  // between a linear sweep and repeatedly picking random numbers with
  // the risk of hitting the same number many times.
  if (s*10<N) {
    boost::dynamic_bitset<uint64_t> check(N,0);
    for (size_t i = 0; i < v.size(); i++) {
      size_t x = util::rand_excl(N);
      while (check[x]) x = util::rand_excl(N);
      check[x]=true;
      v[i] = x;
    }
  } else {
    size_t m=0;
    for (size_t t = 0; m <= s && t < N; t++)
      if (s==N || util::rand_excl(N-t) < s-m) v[m++] = t;
  }
}

};

#endif


#include "util/probing_hash_table.hh"
#include "util/scoped.hh"
#include "util/usage.hh"

#include <boost/random/mersenne_twister.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION / 100000 < 1
#error BOOST_LIB_VERSION is to old. Time to upgrade.
#elif BOOST_VERSION / 100000 > 1 || BOOST_VERSION / 100 % 1000 > 46
#include <boost/random/uniform_int_distribution.hpp>
#define have_uniform_int_distribution 
#else
#include <boost/random/uniform_int.hpp>
#endif

#include <iostream>

namespace util {
namespace {

struct Entry {
  typedef uint64_t Key;
  Key key;
  Key GetKey() const { return key; }
};

typedef util::ProbingHashTable<Entry, util::IdentityHash> Table;

void Test(uint64_t entries, uint64_t lookups, float multiplier = 1.5) {
  std::size_t size = Table::Size(entries, multiplier);
  scoped_malloc backing(util::CallocOrThrow(size));
  Table table(backing.get(), size);
#ifdef have_uniform_int_distribution
  boost::random::mt19937 gen;
  boost::random::uniform_int_distribution<> dist(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max());
#else
  boost::mt19937 gen;
  boost::uniform_int<> dist(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max());
#endif
  double start = UserTime();
  for (uint64_t i = 0; i < entries; ++i) {
    Entry entry;
    entry.key = dist(gen);
    table.Insert(entry);
  }
  double inserted = UserTime();
  bool meaningless = true;
  for (uint64_t i = 0; i < lookups; ++i) {
    Table::ConstIterator it;
    meaningless ^= table.Find(dist(gen), it);
  }
  std::cout << meaningless << ' ' << entries << ' ' << multiplier << ' ' << (inserted - start) << ' ' << (UserTime() - inserted) / static_cast<double>(lookups) << std::endl;
}

} // namespace
} // namespace util

int main() {
  for (uint64_t i = 1; i <= 10000000ULL; i *= 10) {
    util::Test(i, 10000000);
  }
}

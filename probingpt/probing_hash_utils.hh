#pragma once

#include "util/probing_hash_table.hh"

#if defined(_WIN32) || defined(_WIN64)
#include <mman.h>
#else
#include <sys/mman.h>
#endif
#include <boost/functional/hash.hpp>
#include <fcntl.h>
#include <fstream>

namespace probingpt
{

#define API_VERSION 15

//Hash table entry
struct Entry {
  typedef uint64_t Key;
  Key key;

  Key GetKey() const {
    return key;
  }

  void SetKey(Key to) {
    key = to;
  }

  uint64_t value;
};

#define NONE       std::numeric_limits<uint64_t>::max()

//Define table
typedef util::ProbingHashTable<Entry, boost::hash<uint64_t> > Table;

void serialize_table(char *mem, size_t size, const std::string &filename);

char * readTable(const char * filename, util::LoadMethod load_method, util::scoped_fd &file, util::scoped_memory &memory);

uint64_t getKey(const uint64_t source_phrase[], size_t size);

struct TargetPhraseInfo {
  uint32_t alignTerm;
  uint32_t alignNonTerm;
  uint16_t numWords;
  uint16_t propLength;
  uint16_t filler;
};

}


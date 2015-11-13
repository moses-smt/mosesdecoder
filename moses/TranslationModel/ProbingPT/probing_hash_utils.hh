#pragma once

#include "util/probing_hash_table.hh"

#include <sys/mman.h>
#include <boost/functional/hash.hpp>
#include <fcntl.h>
#include <fstream>


//Hash table entry
struct Entry {
  uint64_t key;
  typedef uint64_t Key;
  unsigned int bytes_toread;

  uint64_t GetKey() const {
    return key;
  }

  void SetKey(uint64_t to) {
    key = to;
  }

  uint64_t GetValue() const {
    return value;
  }

  uint64_t value;
};

//Define table
typedef util::ProbingHashTable<Entry, boost::hash<uint64_t> > Table;

void serialize_table(char *mem, size_t size, const char * filename);

char * readTable(const char * filename, size_t size);

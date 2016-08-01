#pragma once

#include "util/probing_hash_table.hh"

#include <sys/mman.h>
#include <boost/functional/hash.hpp>
#include <fcntl.h>
#include <fstream>

namespace Moses2
{

#define API_VERSION 15

//Hash table entry
struct Entry
{
  typedef uint64_t Key;
  Key key;

  Key GetKey() const
  {
    return key;
  }

  void SetKey(Key to)
  {
    key = to;
  }

  uint64_t value;
};

#define NONE       std::numeric_limits<uint64_t>::max()

//Define table
typedef util::ProbingHashTable<Entry, boost::hash<uint64_t> > Table;

void serialize_table(char *mem, size_t size, const std::string &filename);

char * readTable(const char * filename, size_t size);

uint64_t getKey(const uint64_t source_phrase[], size_t size);

struct TargetPhraseInfo
{
  uint32_t alignTerm;
  uint32_t alignNonTerm;
  uint16_t numWords;
  uint16_t propLength;
  uint16_t filler;
};

}


#pragma once

#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <queue>
#include <sys/stat.h> //mkdir

#include "hash.hh" //Includes line_splitter
#include "probing_hash_utils.hh"

#include "util/file_piece.hh"
#include "util/file.hh"
#include "vocabid.hh"

namespace Moses2
{

void createProbingPT(const std::string &phrasetable_path,
    const std::string &basepath, int num_scores, int num_lex_scores,
    bool log_prob, int max_cache_size, bool scfg);
uint64_t getKey(const std::vector<uint64_t> &source_phrase);

void InsertPrefixes(const std::vector<uint64_t> &vocabid_source, const std::vector<uint64_t> &prevVocabid_source);
std::vector<uint64_t> CreatePrefix(const std::vector<uint64_t> &vocabid_source, size_t endPos);

template<typename T>
std::string Debug(const std::vector<T> &vec)
{
  std::stringstream strm;
  for (size_t i = 0; i < vec.size(); ++i) {
    strm << vec[i] << " ";
  }
  return strm.str();
}

size_t countUniqueSource(const std::string &path);

class CacheItem
{
public:
  std::string source;
  float count;
  CacheItem(const std::string &source, float count) :
      source(source), count(count)
  {
  }

  bool operator<(const CacheItem &other) const
  {
    return count > other.count;
  }
};

class CacheItemOrderer
{
public:
  bool operator()(const CacheItem* a, const CacheItem* b) const
  {
    return (*a) < (*b);
  }
};

void serialize_cache(
    std::priority_queue<CacheItem*, std::vector<CacheItem*>, CacheItemOrderer> &cache,
    const std::string &path, float totalSourceCount);

}


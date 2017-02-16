#pragma once

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <queue>
#include <sys/stat.h> //mkdir

#include "hash.h" //Includes line_splitter
#include "probing_hash_utils.h"
#include "vocabid.h"

#include "util/file_piece.hh"
#include "util/file.hh"

namespace probingpt
{
typedef std::vector<uint64_t> SourcePhrase;


class Node
{
  typedef boost::unordered_map<uint64_t, Node> Children;
  Children m_children;

public:
  uint64_t key;
  bool done;

  Node()
    :done(false)
  {}

  void Add(Table &table, const SourcePhrase &sourcePhrase, size_t pos = 0);
  void Write(Table &table);
};


void createProbingPT(const std::string &phrasetable_path,
                     const std::string &basepath, int num_scores, int num_lex_scores,
                     bool log_prob, int max_cache_size, bool scfg);
uint64_t getKey(const std::vector<uint64_t> &source_phrase);

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
  uint64_t sourceKey;
  float count;
  CacheItem(const std::string &vSource, uint64_t vSourceKey, float vCount)
    :source(vSource)
    ,sourceKey(vSourceKey)
    ,count(vCount) {
  }

  bool operator<(const CacheItem &other) const {
    return count > other.count;
  }
};

class CacheItemOrderer
{
public:
  bool operator()(const CacheItem* a, const CacheItem* b) const {
    return (*a) < (*b);
  }
};

void serialize_cache(
  std::priority_queue<CacheItem*, std::vector<CacheItem*>, CacheItemOrderer> &cache,
  const std::string &path, float totalSourceCount);

}


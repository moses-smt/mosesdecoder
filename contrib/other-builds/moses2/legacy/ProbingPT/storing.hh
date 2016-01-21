#pragma once

#include <cstdio>
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

void createProbingPT(
		const std::string &phrasetable_path,
		const std::string &basepath,
        int num_scores,
		int num_lex_scores,
		bool log_prob,
		int max_cache_size);

size_t countUniqueSource(const std::string &path);

class CacheItem
{
public:
	std::string source;
	float count;
	CacheItem(const std::string &source, float count)
	:source(source)
	,count(count)
	{}

	bool operator<(const CacheItem &other) const
	{
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

void serialize_cache(std::priority_queue<CacheItem*, std::vector<CacheItem*>, CacheItemOrderer> &cache,
		const std::string &path,
		float totalSourceCount);

}



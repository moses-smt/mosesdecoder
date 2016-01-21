#pragma once

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <queue>
#include <sys/stat.h> //mkdir

#include "hash.hh" //Includes line_splitter
#include "probing_hash_utils.hh"
#include "huffmanish.hh"

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

class BinaryFileWriter
{
  std::vector<unsigned char> binfile;
  std::vector<unsigned char>::iterator it;
  //Output binary
  std::ofstream os;

public:
  unsigned int dist_from_start; //Distance from the start of the vector.
  uint64_t extra_counter; //After we reset the counter, we still want to keep track of the correct offset, so

  BinaryFileWriter (std::string);
  ~BinaryFileWriter ();
  void write (std::vector<unsigned char> * bytes);
  void flush (); //Flush to disk

};

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



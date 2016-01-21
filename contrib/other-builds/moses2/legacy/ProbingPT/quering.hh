#pragma once

#include <sys/stat.h> //For finding size of file
#include "vocabid.hh"
#include <algorithm> //toLower
#include <deque>
#include "probing_hash_utils.hh"
#include "hash.hh" //Includes line splitter
#include "line_splitter.hh"
#include "../../Vector.h"

namespace Moses2
{

char * read_binary_file(char * filename);

class QueryEngine
{
  const std::map<unsigned int, std::string> *vocabids;
  std::map<uint64_t, std::string> source_vocabids;

  Table table;
  char *mem; //Memory for the table, necessary so that we can correctly destroy the object

  size_t table_filesize;
  bool is_reordering;

public:
  int num_scores;
  int num_lex_scores;
  bool logProb;

  QueryEngine (const char *);
  ~QueryEngine();

  std::pair<bool, uint64_t> query(uint64_t key);

  const std::map<uint64_t, std::string> &getSourceVocab() const {
    return source_vocabids;
  }

  uint64_t getKey(uint64_t source_phrase[], size_t size) const;

};

}



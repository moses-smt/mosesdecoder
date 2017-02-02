#pragma once

#include <boost/unordered_map.hpp>
#include <sys/stat.h> //For finding size of file
#include "vocabid.hh"
#include <algorithm> //toLower
#include <deque>
#include "probing_hash_utils.hh"
#include "hash.hh" //Includes line splitter
#include "line_splitter.hh"
#include "moses//Util.h"

namespace Moses
{

class QueryEngine
{
  std::map<uint64_t, std::string> source_vocabids;

  typedef std::vector<unsigned char> Alignments;
  std::vector<Alignments> alignColl;

  Table table;
  char *mem; //Memory for the table, necessary so that we can correctly destroy the object

  size_t table_filesize;
  bool is_reordering;

  void read_alignments(const std::string &alignPath);

public:
  int num_scores;
  int num_lex_scores;
  bool logProb;

  QueryEngine(const char *);
  ~QueryEngine();

  std::pair<bool, uint64_t> query(uint64_t key);

  const std::map<uint64_t, std::string> &getSourceVocab() const {
    return source_vocabids;
  }

  const std::vector<Alignments> &getAlignments() const {
    return alignColl;
  }

  uint64_t getKey(uint64_t source_phrase[], size_t size) const;

  template<typename T>
  inline bool Get(const boost::unordered_map<std::string, std::string> &keyValue, const std::string &sought, T &found) const {
    boost::unordered_map<std::string, std::string>::const_iterator iter = keyValue.find(sought);
    if (iter == keyValue.end()) {
      return false;
    }

    const std::string &foundStr = iter->second;
    found = Scan<T>(foundStr);
    return true;
  }

};

}


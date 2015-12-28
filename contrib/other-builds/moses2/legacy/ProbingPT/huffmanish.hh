#pragma once

//Huffman encodes a line and also produces the vocabulary ids
#include "hash.hh"
#include "line_splitter.hh"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace Moses2
{

//Sorting for the second
struct sort_pair {
  bool operator()(const std::pair<std::string, unsigned int> &left, const std::pair<std::string, unsigned int> &right) {
    return left.second > right.second; //This puts biggest numbers first.
  }
};

struct sort_pair_vec {
  bool operator()(const std::pair<std::vector<unsigned char>, unsigned int> &left, const std::pair<std::vector<unsigned char>, unsigned int> &right) {
    return left.second > right.second; //This puts biggest numbers first.
  }
};

class Huffman
{
  unsigned long uniq_lines; //Unique lines in the file.

  //Containers used when counting the occurence of a given phrase
  std::map<std::string, unsigned int> target_phrase_words;
  std::map<std::vector<unsigned char>, unsigned int> word_all1;

  //Same containers as vectors, for sorting
  std::vector<std::pair<std::string, unsigned int> > target_phrase_words_counts;
  std::vector<std::pair<std::vector<unsigned char>, unsigned int> > word_all1_counts;

  //Huffman maps
  std::map<std::string, unsigned int> target_phrase_huffman;
  std::map<std::vector<unsigned char>, unsigned int> word_all1_huffman;

  //inverted maps
  std::map<unsigned int, std::string> lookup_target_phrase;
  std::map<unsigned int, std::vector<unsigned char> > lookup_word_all1;

public:
  Huffman (const char *);
  void count_elements (const line_text &line);
  void assign_values();
  void serialize_maps(const char * dirname);
  void produce_lookups();

  std::vector<unsigned int> encode_line(const line_text &line);

  //encode line + variable byte ontop
  std::vector<unsigned char> full_encode_line(const line_text &line);

  //Getters
  const std::map<unsigned int, std::string> get_target_lookup_map() const {
    return lookup_target_phrase;
  }
  const std::map<unsigned int, std::vector<unsigned char> > get_word_all1_lookup_map() const {
    return lookup_word_all1;
  }

  unsigned long getUniqLines() {
    return uniq_lines;
  }
};

class HuffmanDecoder
{
  std::map<unsigned int, std::string> lookup_target_phrase;
  std::map<unsigned int, std::vector<unsigned char> > lookup_word_all1;

public:
  HuffmanDecoder (const char *);
  HuffmanDecoder (const std::map<unsigned int, std::string> &, const std::map<unsigned int, std::vector<unsigned char> > &);

  //Getters
  const std::map<unsigned int, std::string> &get_target_lookup_map() const {
    return lookup_target_phrase;
  }
  const std::map<unsigned int, std::vector<unsigned char> > &get_word_all1_lookup_map() const {
    return lookup_word_all1;
  }

  inline const std::string &getTargetWordFromID(unsigned int id);

  std::string getTargetWordsFromIDs(const std::vector<unsigned int> &ids);

  target_text *decode_line (const std::vector<unsigned int> &input, int num_scores, int num_lex_scores);

  //Variable byte decodes a all target phrases contained here and then passes them to decode_line
  std::vector<target_text*> full_decode_line (unsigned char lines[], size_t linesCount, int num_scores, int num_lex_scores);
};

std::string getTargetWordsFromIDs(const std::vector<unsigned int> &ids, const std::map<unsigned int, std::string> &lookup_target_phrase);

inline const std::string &getTargetWordFromID(unsigned int id, const std::map<unsigned int, std::string> &lookup_target_phrase);

inline unsigned int reinterpret_float(float * num);

inline float reinterpret_uint(unsigned int * num);

std::vector<unsigned char> vbyte_encode_line(const std::vector<unsigned int> &line);
inline std::vector<unsigned char> vbyte_encode(unsigned int num);
std::vector<unsigned int> vbyte_decode_line(unsigned char line[], size_t linesSize);
inline unsigned int bytes_to_int(unsigned char number[], size_t numberSize);

}



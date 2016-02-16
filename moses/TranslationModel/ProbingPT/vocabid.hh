//Serialization
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>
#include <iostream>
#include <vector>

#include <map> //Container
#include "hash.hh" //Hash of elements

#include "util/string_piece.hh"  //Tokenization and work with StringPiece
#include "util/tokenize_piece.hh"

void add_to_map(std::map<uint64_t, std::string> *karta, StringPiece textin);

void serialize_map(std::map<uint64_t, std::string> *karta, const char* filename);

void read_map(std::map<uint64_t, std::string> *karta, const char* filename);

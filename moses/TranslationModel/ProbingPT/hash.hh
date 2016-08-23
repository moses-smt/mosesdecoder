#pragma once

#include "util/string_piece.hh"
#include "util/murmur_hash.hh"
#include "util/string_piece.hh"  //Tokenization and work with StringPiece
#include "util/tokenize_piece.hh"
#include <vector>

//Gets the MurmurmurHash for give string
uint64_t getHash(StringPiece text);

std::vector<uint64_t> getVocabIDs(StringPiece textin);

uint64_t getVocabID(std::string candidate);
#pragma once

#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/file_piece.hh"
#include <vector>
#include <cstdlib> //atof
#include "util/string_piece.hh"  //Tokenization and work with StringPiece
#include "util/tokenize_piece.hh"
#include <vector>

namespace Moses2
{

//Struct for holding processed line
struct line_text {
  StringPiece source_phrase;
  StringPiece target_phrase;
  StringPiece prob;
  StringPiece word_align;
  StringPiece counts;
  StringPiece sparse_score;
  StringPiece property;
  std::string property_to_be_binarized;
};

//Struct for holding processed line
struct target_text {
	  std::vector<unsigned int> target_phrase;
	  std::vector<float> prob;
	  std::vector<unsigned char> word_all1;
	  std::vector<char> counts;
	  std::vector<char> sparse_score;
	  std::vector<char> property;

	  void Reset() {
		  target_phrase.clear();
		  prob.clear();
		  word_all1.clear();
		  counts.clear();
		  sparse_score.clear();
		  property.clear();
	  }
};

//Ask if it's better to have it receive a pointer to a line_text struct
line_text splitLine(const StringPiece &textin);

std::vector<unsigned char> splitWordAll1(const StringPiece &textin);

}


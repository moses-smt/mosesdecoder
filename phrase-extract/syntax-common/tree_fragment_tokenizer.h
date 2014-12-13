#pragma once

#include "util/string_piece.hh"

namespace MosesTraining {
namespace Syntax {

enum TreeFragmentTokenType {
  TreeFragmentToken_EOS,
  TreeFragmentToken_LSB,
  TreeFragmentToken_RSB,
  TreeFragmentToken_WORD
};

struct TreeFragmentToken {
 public:
  TreeFragmentToken(TreeFragmentTokenType, StringPiece, std::size_t);
  TreeFragmentTokenType type;
  StringPiece value;
  std::size_t pos;
};

// Tokenizes tree fragment strings in Moses format.
//
// For example, the string "[S [NP [NN weasels]] [VP]]" is tokenized to the
// sequence:
//
//    1   LSB   "["
//    2   WORD  "S"
//    3   LSB   "["
//    4   WORD  "NP"
//    5   LSB   "["
//    6   WORD  "NN"
//    7   WORD  "a"
//    8   RSB   "]"
//    9   RSB   "]"
//    10  LSB   "["
//    11  WORD  "VP"
//    12  RSB   "]"
//    13  RSB   "]"
//    14  EOS   undefined
//
class TreeFragmentTokenizer {
 public:
  TreeFragmentTokenizer();
  TreeFragmentTokenizer(const StringPiece &);

  const TreeFragmentToken &operator*() const { return value_; }
  const TreeFragmentToken *operator->() const { return &value_; }

  TreeFragmentTokenizer &operator++();
  TreeFragmentTokenizer operator++(int);

  friend bool operator==(const TreeFragmentTokenizer &,
                         const TreeFragmentTokenizer &);

  friend bool operator!=(const TreeFragmentTokenizer &,
                         const TreeFragmentTokenizer &);

 private:
  StringPiece str_;
  TreeFragmentToken value_;
  StringPiece::const_iterator iter_;
  StringPiece::const_iterator end_;
  std::size_t pos_;
};

}  // namespace Syntax
}  // namespace MosesTraining

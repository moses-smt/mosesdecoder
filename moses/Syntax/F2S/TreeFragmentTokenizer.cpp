#include "TreeFragmentTokenizer.h"

#include <cctype>

namespace Moses
{
namespace Syntax
{
namespace F2S
{

TreeFragmentToken::TreeFragmentToken(TreeFragmentTokenType t,
                                     StringPiece v, std::size_t p)
  : type(t)
  , value(v)
  , pos(p)
{
}

TreeFragmentTokenizer::TreeFragmentTokenizer()
  : value_(TreeFragmentToken_EOS, "", -1)
{
}

TreeFragmentTokenizer::TreeFragmentTokenizer(const StringPiece &s)
  : str_(s)
  , value_(TreeFragmentToken_EOS, "", -1)
  , iter_(s.begin())
  , end_(s.end())
  , pos_(0)
{
  ++(*this);
}

TreeFragmentTokenizer &TreeFragmentTokenizer::operator++()
{
  while (iter_ != end_ && (*iter_ == ' ' || *iter_ == '\t')) {
    ++iter_;
    ++pos_;
  }

  if (iter_ == end_) {
    value_ = TreeFragmentToken(TreeFragmentToken_EOS, "", pos_);
    return *this;
  }

  if (*iter_ == '[') {
    value_ = TreeFragmentToken(TreeFragmentToken_LSB, "[", pos_);
    ++iter_;
    ++pos_;
  } else if (*iter_ == ']') {
    value_ = TreeFragmentToken(TreeFragmentToken_RSB, "]", pos_);
    ++iter_;
    ++pos_;
  } else {
    std::size_t start = pos_;
    while (true) {
      ++iter_;
      ++pos_;
      if (iter_ == end_ || *iter_ == ' ' || *iter_ == '\t') {
        break;
      }
      if (*iter_ == '[' || *iter_ == ']') {
        break;
      }
    }
    StringPiece word = str_.substr(start, pos_-start);
    value_ = TreeFragmentToken(TreeFragmentToken_WORD, word, start);
  }

  return *this;
}

TreeFragmentTokenizer TreeFragmentTokenizer::operator++(int)
{
  TreeFragmentTokenizer tmp(*this);
  ++*this;
  return tmp;
}

bool operator==(const TreeFragmentTokenizer &lhs,
                const TreeFragmentTokenizer &rhs)
{
  if (lhs.value_.type == TreeFragmentToken_EOS ||
      rhs.value_.type == TreeFragmentToken_EOS) {
    return lhs.value_.type == TreeFragmentToken_EOS &&
           rhs.value_.type == TreeFragmentToken_EOS;
  }
  return lhs.iter_ == rhs.iter_;
}

bool operator!=(const TreeFragmentTokenizer &lhs,
                const TreeFragmentTokenizer &rhs)
{
  return !(lhs == rhs);
}

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses

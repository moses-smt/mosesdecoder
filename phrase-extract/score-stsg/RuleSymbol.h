#pragma once

#include "util/string_piece.hh"

namespace Moses
{
namespace ScoreStsg
{

struct RuleSymbol
{
  StringPiece value;
  bool isNonTerminal;
};

}  // namespace ScoreStsg
}  // namespace Moses

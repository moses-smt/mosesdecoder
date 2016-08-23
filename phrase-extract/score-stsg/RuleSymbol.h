#pragma once

#include "util/string_piece.hh"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

struct RuleSymbol {
  StringPiece value;
  bool isNonTerminal;
};

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining

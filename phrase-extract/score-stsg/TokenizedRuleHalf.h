#pragma once

#include <string>
#include <vector>

#include "syntax-common/tree_fragment_tokenizer.h"

#include "RuleSymbol.h"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

// Stores one half of a STSG rule, as represented in the extract file.  The
// original string is stored as the member 'string', along with its token
// sequence ('tokens') and frontier symbol sequence ('frontierSymbols').  Note
// that 'tokens' and 'frontierSymbols' use StringPiece objects that depend on
// the original string.  Therefore changing the value of 'string' invalidates
// both 'tokens' and 'frontierSymbols'.
struct TokenizedRuleHalf {
  bool IsFullyLexical() const;
  bool IsString() const;
  bool IsTree() const;

  // The rule half as it appears in the extract file, except with any trailing
  // or leading spaces removed (here a space is defined as a blank or a tab).
  std::string string;

  // The token sequence for the string.
  std::vector<TreeFragmentToken> tokens;

  // The frontier symbols of the rule half.  For example:
  //
  // string:    "[VP [VBN] [PP [IN] [NP [DT] [JJ positive] [NN light]]]]"
  // frontier:  ("VBN",t), ("IN",t), ("DT",t), ("positive",f), ("light",f)
  //
  // string:  "[X] [X] Sinne [X]"
  // frontier:  ("X",t), ("X",t), ("Sinne",f), ("X",t)
  //
  std::vector<RuleSymbol> frontierSymbols;
};

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining

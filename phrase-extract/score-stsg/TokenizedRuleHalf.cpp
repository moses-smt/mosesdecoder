#include "TokenizedRuleHalf.h"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

bool TokenizedRuleHalf::IsFullyLexical() const
{
  for (std::vector<RuleSymbol>::const_iterator p = frontierSymbols.begin();
       p != frontierSymbols.end(); ++p) {
    if (p->isNonTerminal) {
      return false;
    }
  }
  return true;
}

bool TokenizedRuleHalf::IsString() const
{
  // A rule half is either a string (like "[X] and [X]") or a tree (like
  // "[NP [NP] [CC and] [NP]]").
  //
  // A string must start with a terminal or a non-terminal (in square brackets).
  // A tree must start with '[' followed by a word then either another word or
  // another '['.
  return (tokens[0].type == TreeFragmentToken_WORD ||
          tokens[2].type == TreeFragmentToken_RSB);
}

bool TokenizedRuleHalf::IsTree() const
{
  return !IsString();
}

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining

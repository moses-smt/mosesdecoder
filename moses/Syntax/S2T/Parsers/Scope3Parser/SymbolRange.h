#pragma once

namespace Moses
{
namespace Syntax
{
namespace S2T
{

// Describes the range of possible start and end positions for a symbol
// belonging to a node in a PatternApplicationTrie.
struct SymbolRange {
  int minStart;
  int maxStart;
  int minEnd;
  int maxEnd;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

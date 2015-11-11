#pragma once

#include "RuleTrie.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

// Base for classes that create a RuleTrie (currently RuleTrieLoader and
// OovHandler).  RuleTrieCreator is a friend of RuleTrie.
class RuleTrieCreator
{
protected:
  // Provide access to RuleTrie's private SortAndPrune function.
  void SortAndPrune(RuleTrie &trie, std::size_t limit) {
    trie.SortAndPrune(limit);
  }

  // Provide access to RuleTrie's private
  // GetOrCreateTargetPhraseCollection function.
  TargetPhraseCollection::shared_ptr GetOrCreateTargetPhraseCollection(
    RuleTrie &trie, const Word &sourceLHS, const Phrase &sourceRHS) {
    return trie.GetOrCreateTargetPhraseCollection(sourceLHS, sourceRHS);
  }
};

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses

#pragma once

#include "RuleTrie.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
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

  // Provide access to RuleTrie's private GetOrCreateTargetPhraseCollection
  // function.
  TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection
  ( RuleTrie &trie, const Phrase &source, const TargetPhrase &target,
    const Word *sourceLHS) {
    return trie.GetOrCreateTargetPhraseCollection(source, target, sourceLHS);
  }
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

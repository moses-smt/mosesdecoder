#pragma once

#include "HyperTree.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// Base for classes that create a HyperTree (currently HyperTreeLoader and
// GlueRuleSynthesizer).  HyperTreeCreator is a friend of HyperTree.
class HyperTreeCreator
{
protected:
  // Provide access to HyperTree's private SortAndPrune function.
  void SortAndPrune(HyperTree &trie, std::size_t limit) {
    trie.SortAndPrune(limit);
  }

  // Provide access to HyperTree's private GetOrCreateTargetPhraseCollection
  // function.
  TargetPhraseCollection::shared_ptr GetOrCreateTargetPhraseCollection(
    HyperTree &trie, const HyperPath &fragment) {
    return trie.GetOrCreateTargetPhraseCollection(fragment);
  }
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses

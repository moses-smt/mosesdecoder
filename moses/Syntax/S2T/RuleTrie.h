#pragma once

#include <cstddef>

#include "moses/Syntax/RuleTable.h"

namespace Moses
{

class Phrase;
class TargetPhrase;
class TargetPhraseCollection;
class Word;

namespace Syntax
{
namespace S2T
{

// Base class for parser-specific trie types.
class RuleTrie : public RuleTable
{
public:
  RuleTrie(const RuleTableFF *ff) : RuleTable(ff) {}

  virtual bool HasPreterminalRule(const Word &) const = 0;

private:
  friend class RuleTrieCreator;

  virtual TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection(const Phrase &source,
                                    const TargetPhrase &target,
                                    const Word *sourceLHS) = 0;

  virtual void SortAndPrune(std::size_t) = 0;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

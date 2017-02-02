#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "syntax-common/numbered_set.h"
#include "syntax-common/tree.h"
#include "syntax-common/tree_fragment_tokenizer.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// Base class for TreeTsgFilter and ForestTsgFilter, both of which filter rule
// tables where the source-side is TSG.
class TsgFilter
{
public:
  virtual ~TsgFilter() {}

  // Read a rule table from 'in' and filter it according to the test sentences.
  void Filter(std::istream &in, std::ostream &out);

protected:
  // Maps symbols (terminals and non-terminals) from strings to integers.
  typedef NumberedSet<std::string, std::size_t> Vocabulary;

  // Represents a tree using integer vocabulary values.
  typedef Tree<Vocabulary::IdType> IdTree;

  // Build an IdTree (wrt m_testVocab) for the tree beginning at position i of
  // the token sequence or return 0 if any symbol in the fragment is not in
  // m_testVocab.  If successful then on return, i will be set to the position
  // immediately after the last token of the tree and leaves will contain the
  // pointers to the fragment's leaves.  If the build fails then i and leaves
  // are undefined.
  IdTree *BuildTree(const std::vector<TreeFragmentToken> &tokens, int &i,
                    std::vector<IdTree *> &leaves);

  // Try to match a fragment.  The implementation depends on whether the test
  // sentences are trees or forests.
  virtual bool MatchFragment(const IdTree &, const std::vector<IdTree *> &) = 0;

  // The symbol vocabulary of the test sentences.
  Vocabulary m_testVocab;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining

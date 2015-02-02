#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "syntax-common/numbered_set.h"
#include "syntax-common/string_tree.h"
#include "syntax-common/tree.h"
#include "syntax-common/tree_fragment_tokenizer.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// Filters a rule table (currently assumed to be tree-to-string, STSG),
// discarding rules that cannot be applied to a given set of test sentences.
class TreeBasedFilter
{
public:
  // Initialize the filter for a given set of test sentences.
  TreeBasedFilter(const std::vector<boost::shared_ptr<StringTree> > &);

  // Read a rule table from 'in' and filter it according to the test sentences.
  // This is slow because it involves testing every rule (or a significant
  // fraction) at every node of every test sentence parse tree.  There are a
  // couple of optimizations that speed things up in practice, but it could
  // still use some work to make it faster.
  //
  // Some statistics from real data (WMT14, English-German):
  //
  //  4.4M    Parallel sentences (source-side parsed with Berkeley parser)
  //  2.7K    Test sentences (newstest2014)
  //
  // 73.4M    Original rule table size (number of distinct, composed GHKM rules)
  // 22.9M    Number of rules with same source-side as previous rule
  // 50.5M    Number of rules requiring vocabulary matching test
  // 24.1M    Number of rules requiring full tree matching test
  //  6.7M    Number of rules retained after filtering
  //
  void Filter(std::istream &in, std::ostream &out);

private:
  // Maps source-side symbols (terminals and non-terminals) from strings to
  // integers.
  typedef NumberedSet<std::string, std::size_t> Vocabulary;

  // Represents the test trees using their integer vocabulary values for faster
  // matching.
  typedef Tree<Vocabulary::IdType> IdTree;

  // Add an entry to m_labelToTree for every subtree of the given tree.
  void AddNodesToMap(const IdTree &);

  // Build an IdTree (wrt m_testVocab) for the tree beginning at position i of
  // the token sequence or return 0 if any symbol in the fragment is not in
  // m_testVocab.  If successful then on return, i will be set to the position
  // immediately after the last token of the tree and leaves will contain the
  // pointers to the fragment's leaves.  If the build fails then i and leaves
  // are undefined.
  IdTree *BuildTree(const std::vector<TreeFragmentToken> &tokens, int &i,
                    std::vector<IdTree *> &leaves);

  // Try to match a fragment against any test tree.
  bool MatchFragment(const IdTree &, const std::vector<IdTree *> &);

  // Try to match a fragment against a specific subtree of a test tree.
  bool MatchFragment(const IdTree &, const IdTree &);

  // Convert a StringTree to an IdTree (wrt m_testVocab).  Inserts symbols into
  // m_testVocab.
  IdTree *StringTreeToIdTree(const StringTree &);

  std::vector<boost::shared_ptr<IdTree> > m_sentences;
  std::vector<std::vector<const IdTree *> > m_labelToTree;
  Vocabulary m_testVocab;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining

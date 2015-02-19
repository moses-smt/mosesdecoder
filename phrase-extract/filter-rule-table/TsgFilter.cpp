#include "TsgFilter.h"

#include "boost/scoped_ptr.hpp"

#include "util/string_piece.hh"
#include "util/string_piece_hash.hh"
#include "util/tokenize_piece.hh"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// Read a rule table from 'in' and filter it according to the test sentences.
//
// This involves testing TSG fragments for matches against at potential match
// sites in the set of test parse trees / forests.  There are a few
// optimizations that make this reasonably fast in practice:
//
// Optimization 1
// If a rule has the same TSG fragment as the previous rule then re-use the
// result of the previous filtering decision.
//
// Optimization 2
// Test if the TSG fragment contains any symbols that don't occur in the
// symbol vocabulary of the test set.  If it does then the rule can be
// discarded.
//
// Optimization 3
// Prior to filtering, a map is constructed from each distinct test set tree /
// forest vertex symbol to the set of vertices having that symbol.  During
// filtering, for each rule's TSG fragment the leaf with the smallest number of
// corresponding test nodes nodes is determined.  Matching is only attempted
// at those sites (this is done in MatchFragment, which has tree- and
// forest-specific implementations).
//
// Some statistics from real data (WMT14, English-German, tree-version):
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
void TsgFilter::Filter(std::istream &in, std::ostream &out)
{
  const util::MultiCharacter delimiter("|||");

  std::string line;
  std::string prevLine;
  StringPiece source;
  bool keep = true;
  int lineNum = 0;
  std::vector<TreeFragmentToken> tokens;
  std::vector<IdTree *> leaves;

  while (std::getline(in, line)) {
    ++lineNum;

    // Read the source-side of the rule.
    util::TokenIter<util::MultiCharacter> it(line, delimiter);

    // Check if this rule has the same source-side as the previous rule.  If
    // it does then we already know whether or not to keep the rule.  This
    // optimisation is based on the assumption that the rule table is sorted
    // (which is the case in the standard Moses training pipeline).
    if (*it == source) {
      if (keep) {
        out << line << std::endl;
      }
      continue;
    }

    // The source-side is different from the previous rule's.
    source = *it;

    // Tokenize the source-side tree fragment.
    tokens.clear();
    for (TreeFragmentTokenizer p(source); p != TreeFragmentTokenizer(); ++p) {
      tokens.push_back(*p);
    }

    // Construct an IdTree representing the source-side tree fragment.  This
    // will fail if the fragment contains any symbols that don't occur in
    // m_testVocab and in that case the rule can be discarded.  In practice,
    // this catches a lot of discardable rules (see comment at the top of this
    // function).  If the fragment is successfully created then we attempt to
    // match the tree fragment against the test trees.  This test is exact, but
    // slow.
    int i = 0;
    leaves.clear();
    boost::scoped_ptr<IdTree> fragment(BuildTree(tokens, i, leaves));
    keep = fragment.get() && MatchFragment(*fragment, leaves);
    if (keep) {
      out << line << std::endl;
    }

    // Retain line for the next iteration (in order that the source StringPiece
    // remains valid).
    prevLine.swap(line);
  }
}

TsgFilter::IdTree *TsgFilter::BuildTree(
  const std::vector<TreeFragmentToken> &tokens, int &i,
  std::vector<IdTree *> &leaves)
{
  // The subtree starting at tokens[i] is either:
  // 1. a single non-variable symbol (like NP or dog), or
  // 2. a variable symbol (like [NP]), or
  // 3. a subtree with children (like [NP [DT] [NN dog]])

  // First check for case 1.
  if (tokens[i].type == TreeFragmentToken_WORD) {
    Vocabulary::IdType id = m_testVocab.Lookup(tokens[i++].value,
                            StringPieceCompatibleHash(),
                            StringPieceCompatibleEquals());
    if (id == Vocabulary::NullId()) {
      return 0;
    }
    leaves.push_back(new IdTree(id));
    return leaves.back();
  }

  // We must be dealing with either case 2 or 3.  Case 2 looks like case 3 but
  // without the children.
  assert(tokens[i].type == TreeFragmentToken_LSB);

  // Skip over the opening [
  ++i;

  // Read the root symbol of the subtree.
  Vocabulary::IdType id = m_testVocab.Lookup(tokens[i++].value,
                          StringPieceCompatibleHash(),
                          StringPieceCompatibleEquals());
  if (id == Vocabulary::NullId()) {
    return 0;
  }
  IdTree *root = new IdTree(id);

  // Read the children (in case 2 there won't be any).
  while (tokens[i].type != TreeFragmentToken_RSB) {
    IdTree *child = BuildTree(tokens, i, leaves);
    if (!child) {
      delete root;
      return 0;
    }
    root->children().push_back(child);
    child->parent() = root;
  }

  if (root->IsLeaf()) {
    leaves.push_back(root);
  }

  // Skip over the closing ] and we're done.
  ++i;
  return root;
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining

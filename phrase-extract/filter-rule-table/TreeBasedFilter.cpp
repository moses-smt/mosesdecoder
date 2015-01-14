#include "TreeBasedFilter.h"

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

TreeBasedFilter::TreeBasedFilter(
  const std::vector<boost::shared_ptr<StringTree> > &sentences)
{
  // Convert each StringTree to an IdTree.
  m_sentences.reserve(sentences.size());
  for (std::vector<boost::shared_ptr<StringTree> >::const_iterator p =
         sentences.begin(); p != sentences.end(); ++p) {
    m_sentences.push_back(boost::shared_ptr<IdTree>(StringTreeToIdTree(**p)));
  }

  m_labelToTree.resize(m_testVocab.Size());
  // Construct a map from root labels to IdTree nodes.
  for (std::vector<boost::shared_ptr<IdTree> >::const_iterator p =
         m_sentences.begin(); p != m_sentences.end(); ++p) {
    AddNodesToMap(**p);
  }
}

TreeBasedFilter::IdTree *TreeBasedFilter::StringTreeToIdTree(
  const StringTree &s)
{
  IdTree *t = new IdTree(m_testVocab.Insert(s.value()));
  const std::vector<StringTree*> &sChildren = s.children();
  std::vector<IdTree*> &tChildren = t->children();
  tChildren.reserve(sChildren.size());
  for (std::vector<StringTree*>::const_iterator p = sChildren.begin();
       p != sChildren.end(); ++p) {
    IdTree *child = StringTreeToIdTree(**p);
    child->parent() = t;
    tChildren.push_back(child);
  }
  return t;
}

void TreeBasedFilter::AddNodesToMap(const IdTree &tree)
{
  m_labelToTree[tree.value()].push_back(&tree);
  const std::vector<IdTree*> &children = tree.children();
  for (std::vector<IdTree*>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    AddNodesToMap(**p);
  }
}

void TreeBasedFilter::Filter(std::istream &in, std::ostream &out)
{
  const util::MultiCharacter delimiter("|||");

  std::string line;
  std::string prevLine;
  StringPiece source;
  bool keep;
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

bool TreeBasedFilter::MatchFragment(const IdTree &fragment,
                                    const std::vector<IdTree *> &leaves)
{
  typedef std::vector<const IdTree *> TreeVec;

  // Determine which of the fragment's leaves has the smallest number of
  // subtree matches in the test set.  If the fragment contains a rare word
  // (which is pretty likely assuming a Zipfian distribution) then we only
  // have to try matching the fragment against a small number of potential
  // match sites.
  const IdTree *rarestLeaf = leaves[0];
  std::size_t lowestCount = m_labelToTree[rarestLeaf->value()].size();
  for (std::size_t i = 1; i < leaves.size(); ++i) {
    const IdTree *leaf = leaves[i];
    std::size_t count = m_labelToTree[leaf->value()].size();
    if (count < lowestCount) {
      lowestCount = count;
      rarestLeaf = leaf;
    }
  }

  // Determine the depth of the chosen leaf.
  const std::size_t depth = rarestLeaf->Depth();

  // Try to match the rule fragment against the test set subtrees where a
  // leaf match was found.
  TreeVec &nodes = m_labelToTree[rarestLeaf->value()];
  for (TreeVec::const_iterator p = nodes.begin(); p != nodes.end(); ++p) {
    // Navigate 'depth' positions up the subtree to find the root of the
    // potential match site.
    const IdTree *t = *p;
    std::size_t d = depth;
    while (d && t->parent()) {
      t = t->parent();
      --d;
    }
    if (d > 0) {
      // The potential match site is not tall enough.
      continue;
    }
    if (MatchFragment(fragment, *t)) {
      return true;
    }
  }
  return false;
}

TreeBasedFilter::IdTree *TreeBasedFilter::BuildTree(
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

bool TreeBasedFilter::MatchFragment(const IdTree &fragment, const IdTree &tree)
{
  if (fragment.value() != tree.value()) {
    return false;
  }
  const std::vector<IdTree*> &fragChildren = fragment.children();
  const std::vector<IdTree*> &treeChildren = tree.children();
  if (!fragChildren.empty() && fragChildren.size() != treeChildren.size()) {
    return false;
  }
  for (std::size_t i = 0; i < fragChildren.size(); ++i) {
    if (!MatchFragment(*fragChildren[i], *treeChildren[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining

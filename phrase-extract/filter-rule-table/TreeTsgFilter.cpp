#include "TreeTsgFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

TreeTsgFilter::TreeTsgFilter(
    const std::vector<boost::shared_ptr<StringTree> > &sentences)
{
  // Convert each StringTree to an IdTree.
  m_sentences.reserve(sentences.size());
  for (std::vector<boost::shared_ptr<StringTree> >::const_iterator p =
       sentences.begin(); p != sentences.end(); ++p) {
    m_sentences.push_back(boost::shared_ptr<IdTree>(StringTreeToIdTree(**p)));
  }

  m_labelToTree.resize(m_testVocab.Size());
  // Construct a map from vocabulary Ids to IdTree nodes.
  for (std::vector<boost::shared_ptr<IdTree> >::const_iterator p =
       m_sentences.begin(); p != m_sentences.end(); ++p) {
    AddNodesToMap(**p);
  }
}

TreeTsgFilter::IdTree *TreeTsgFilter::StringTreeToIdTree(const StringTree &s)
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

void TreeTsgFilter::AddNodesToMap(const IdTree &tree)
{
  m_labelToTree[tree.value()].push_back(&tree);
  const std::vector<IdTree*> &children = tree.children();
  for (std::vector<IdTree*>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    AddNodesToMap(**p);
  }
}

bool TreeTsgFilter::MatchFragment(const IdTree &fragment,
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

bool TreeTsgFilter::MatchFragment(const IdTree &fragment, const IdTree &tree)
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

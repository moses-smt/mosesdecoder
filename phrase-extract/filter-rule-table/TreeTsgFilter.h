#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "SyntaxTree.h"

#include "syntax-common/numbered_set.h"
#include "syntax-common/tree.h"
#include "syntax-common/tree_fragment_tokenizer.h"

#include "TsgFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// Filters a rule table, discarding rules that cannot be applied to a given
// test set.  The rule table must have a TSG source-side and the test sentences
// must be parse trees.
class TreeTsgFilter : public TsgFilter
{
public:
  // Initialize the filter for a given set of test sentences.
  TreeTsgFilter(const std::vector<boost::shared_ptr<SyntaxTree> > &);

private:
  // Add an entry to m_labelToTree for every subtree of the given tree.
  void AddNodesToMap(const IdTree &);

  // Tree-specific implementation of virtual function.
  bool MatchFragment(const IdTree &, const std::vector<IdTree *> &);

  // Try to match a fragment against a specific subtree of a test tree.
  bool MatchFragment(const IdTree &, const IdTree &);

  // Convert a SyntaxTree to an IdTree (wrt m_testVocab).  Inserts symbols into
  // m_testVocab.
  IdTree *SyntaxTreeToIdTree(const SyntaxTree &);

  std::vector<boost::shared_ptr<IdTree> > m_sentences;
  std::vector<std::vector<const IdTree *> > m_labelToTree;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining

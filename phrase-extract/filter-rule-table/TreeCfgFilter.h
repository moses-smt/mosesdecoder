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

#include "CfgFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// Filters a rule table, discarding rules that cannot be applied to a given
// test set.  The rule table must have a TSG source-side and the test sentences
// must be parse trees.
class TreeCfgFilter : public CfgFilter
{
public:
  // Initialize the filter for a given set of test sentences.
  TreeCfgFilter(const std::vector<boost::shared_ptr<SyntaxTree> > &);

  void Filter(std::istream &in, std::ostream &out);
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining

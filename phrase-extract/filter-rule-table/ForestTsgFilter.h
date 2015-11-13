#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "syntax-common/numbered_set.h"
#include "syntax-common/tree.h"
#include "syntax-common/tree_fragment_tokenizer.h"

#include "Forest.h"
#include "StringForest.h"
#include "TsgFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// Filters a rule table, discarding rules that cannot be applied to a given
// test set.  The rule table must have a TSG source-side and the test sentences
// must be parse forests.
class ForestTsgFilter : public TsgFilter
{
public:
  // Initialize the filter for a given set of test forests.
  ForestTsgFilter(const std::vector<boost::shared_ptr<StringForest> > &);

private:
  struct IdForestValue {
    Vocabulary::IdType id;
    std::size_t start;
    std::size_t end;
  };

  static const std::size_t kMatchLimit;

  // Represents a forest using integer vocabulary values.
  typedef Forest<IdForestValue> IdForest;

  typedef boost::unordered_map<std::size_t,
          std::vector<const IdForest::Vertex*> > InnerMap;

  typedef std::vector<InnerMap> IdToSentenceMap;

  // Forest-specific implementation of virtual function.
  bool MatchFragment(const IdTree &, const std::vector<IdTree *> &);

  // Try to match a fragment against a specific vertex of a test forest.
  bool MatchFragment(const IdTree &, const IdForest::Vertex &);

  // Convert a StringForest to an IdForest (wrt m_testVocab).  Inserts symbols
  // into m_testVocab.
  boost::shared_ptr<IdForest> StringForestToIdForest(const StringForest &);

  std::vector<boost::shared_ptr<IdForest> > m_sentences;
  IdToSentenceMap m_idToSentence;
  std::size_t m_matchCount;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "util/string_piece.hh"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

// A group of rules that share the same source-side.  Rules are added through
// calls to SetNewSource() and AddRule().  They can then be accessed via the
// iterators.
//
// It is assumed that rules with the same (target, ntAlign, alignment) value
// will be added consecutively, and so will rules with the same
// (target, ntAlign) value.  In other words, it is assumed that rules will be
// added in the order they occur in a correctly-sorted extract file.
class RuleGroup
{
public:
  // Stores the target-side and NT-alignment of a distinct rule.  Also records
  // the rule's count, the observed symbol alignments (plus their frequencies),
  // and the tree score.
  struct DistinctRule {
    std::string target;
    std::string ntAlign;
    std::vector<std::pair<std::string, int> > alignments;
    int count;
    double treeScore;
  };

  typedef std::vector<DistinctRule>::const_iterator ConstIterator;

  // Begin and End iterators for iterating over the group's distinct rules.
  ConstIterator Begin() const {
    return m_distinctRules.begin();
  }
  ConstIterator End() const {
    return m_distinctRules.end();
  }

  // Get the current source-side value.
  const std::string &GetSource() const {
    return m_source;
  }

  // Get the number of distinct rules.
  int GetSize() const {
    return m_distinctRules.size();
  }

  // Get the total count.
  int GetTotalCount() const {
    return m_totalCount;
  }

  // Clear the rule group and set a new source-side value.  This must be
  // done once for every new source-side value, prior to the first call to
  // AddRule().
  void SetNewSource(const StringPiece &source);

  // Add a rule.  To determine rule distinctness, the target and ntAlign
  // values will be checked against those of the previous rule only (in other
  // words, the input is assumed to be ordered).
  void AddRule(const StringPiece &target, const StringPiece &ntAlign,
               const StringPiece &fullAlign, int count, double treeScore);

private:
  std::string m_source;
  std::vector<DistinctRule> m_distinctRules;
  int m_totalCount;
};

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining

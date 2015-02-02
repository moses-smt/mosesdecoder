#pragma once

#include "moses/Syntax/PHyperedge.h"

#include "Forest.h"
#include "HyperTree.h"
#include "RuleMatcher.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// Rule matcher based on the algorithm from this paper:
//
//  Hui Zhang, Min Zhang, Haizhou Li, and Chew Lim Tan
//  "Fast Translation Rule Matching for Syntax-based Statistical Machine
//   Translation"
//  In proceedings of EMNLP 2009
//
template<typename Callback>
class RuleMatcherHyperTree : public RuleMatcher<Callback>
{
 public:
  RuleMatcherHyperTree(const HyperTree &);

  ~RuleMatcherHyperTree() {}

  void EnumerateHyperedges(const Forest::Vertex &, Callback &);

 private:
  // Frontier node sequence.
  typedef std::vector<const Forest::Vertex *> FNS;

  // An AnnotatedFNS is a FNS annotated with the set of forest hyperedges that
  // constitute the tree fragment from which it was derived.
  struct AnnotatedFNS {
    FNS fns;
    std::vector<const Forest::Hyperedge *> fragment;
  };

  // A MatchItem is like the FP structure in Zhang et al. (2009), but it also
  // records the set of forest hyperedges that constitute the matched tree
  // fragment.
  struct MatchItem {
    AnnotatedFNS annotatedFNS;
    const HyperTree::Node *trieNode;
  };

  // Implements the Cartsian product operation from line 16 of Algorithm 4
  // (Zhang et al., 2009), which in this implementation also involves
  // combining the fragment information associated with the FNS objects.
  void CartesianProduct(const std::vector<AnnotatedFNS> &,
                        const std::vector<AnnotatedFNS> &,
                        std::vector<AnnotatedFNS> &);

  int CountCommas(const HyperPath::NodeSeq &);

  bool MatchChildren(const std::vector<Forest::Vertex *> &,
                     const HyperPath::NodeSeq &, std::size_t, std::size_t);

  void PropagateNextLexel(const MatchItem &);

  int SubSeqLength(const HyperPath::NodeSeq &, int);

  const HyperTree &m_ruleTrie;
  PHyperedge m_hyperedge;
  std::queue<MatchItem> m_queue;  // Called "SFP" in Zhang et al. (2009)
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses

// Implementation
#include "RuleMatcherHyperTree-inl.h"

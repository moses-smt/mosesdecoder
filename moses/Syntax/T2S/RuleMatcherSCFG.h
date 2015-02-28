#pragma once

#include "moses/Syntax/PHyperedge.h"

#include "RuleMatcher.h"
#include "RuleTrie.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

// TODO
//
template<typename Callback>
class RuleMatcherSCFG : public RuleMatcher<Callback>
{
public:
  RuleMatcherSCFG(const InputTree &, const RuleTrie &);

  ~RuleMatcherSCFG() {}

  void EnumerateHyperedges(const InputTree::Node &, Callback &);

private:
  bool IsDescendent(const InputTree::Node &, const InputTree::Node &);

  void Match(const InputTree::Node &, const RuleTrie::Node &, int, Callback &);

  const InputTree &m_inputTree;
  const RuleTrie &m_ruleTrie;
  PHyperedge m_hyperedge;
};

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses

// Implementation
#include "RuleMatcherSCFG-inl.h"

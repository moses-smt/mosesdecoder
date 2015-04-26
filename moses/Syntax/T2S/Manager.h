#pragma once

#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "moses/Syntax/F2S/PVertexToStackMap.h"
#include "moses/Syntax/KBestExtractor.h"
#include "moses/Syntax/Manager.h"
#include "moses/Syntax/SVertexStack.h"
#include "moses/TreeInput.h"
#include "moses/Word.h"

#include "InputTree.h"
#include "RuleTrie.h"

namespace Moses
{
namespace Syntax
{

struct SHyperedge;

namespace T2S
{

template<typename RuleMatcher>
class Manager : public Syntax::Manager
{
public:
  Manager(ttasksptr const& ttask);

  void Decode();

  // Get the SHyperedge for the 1-best derivation.
  const SHyperedge *GetBestSHyperedge() const;

  void ExtractKBest(
    std::size_t k,
    std::vector<boost::shared_ptr<KBestExtractor::Derivation> > &kBestList,
    bool onlyDistinct=false) const;

  void OutputDetailedTranslationReport(OutputCollector *collector) const;

private:
  void InitializeRuleMatchers();

  void InitializeStacks();

  void RecombineAndSort(const std::vector<SHyperedge*> &, SVertexStack &);

  InputTree m_inputTree;
  F2S::PVertexToStackMap m_stackMap;
  boost::shared_ptr<RuleTrie> m_glueRuleTrie;
  std::vector<boost::shared_ptr<RuleMatcher> > m_ruleMatchers;
  RuleMatcher *m_glueRuleMatcher;
};

}  // T2S
}  // Syntax
}  // Moses

// Implementation
#include "Manager-inl.h"

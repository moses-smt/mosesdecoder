#pragma once

#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "moses/InputType.h"
#include "moses/Syntax/KBestExtractor.h"
#include "moses/Syntax/Manager.h"
#include "moses/Syntax/SVertexStack.h"
#include "moses/Word.h"

#include "Forest.h"
#include "HyperTree.h"
#include "PVertexToStackMap.h"

namespace Moses
{
namespace Syntax
{

struct SHyperedge;

namespace F2S
{

template<typename RuleMatcher>
class Manager : public Syntax::Manager
{
 public:
  Manager(const InputType &);

  void Decode();

  // Get the SHyperedge for the 1-best derivation.
  const SHyperedge *GetBestSHyperedge() const;

  void ExtractKBest(
      std::size_t k,
      std::vector<boost::shared_ptr<KBestExtractor::Derivation> > &kBestList,
      bool onlyDistinct=false) const;

  void OutputDetailedTranslationReport(OutputCollector *collector) const;

 private:
  const Forest::Vertex &FindRootNode(const Forest &);

  void InitializeRuleMatchers();

  void InitializeStacks();

  void RecombineAndSort(const std::vector<SHyperedge*> &, SVertexStack &);

  boost::shared_ptr<const Forest> m_forest;
  const Forest::Vertex *m_rootVertex;
  PVertexToStackMap m_stackMap;
  boost::shared_ptr<HyperTree> m_glueRuleTrie;
  std::vector<boost::shared_ptr<RuleMatcher> > m_mainRuleMatchers;
  boost::shared_ptr<RuleMatcher> m_glueRuleMatcher;
};

}  // F2S
}  // Syntax
}  // Moses

// Implementation
#include "Manager-inl.h"

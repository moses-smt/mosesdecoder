#pragma once

#include "moses/Syntax/BoundedPriorityContainer.h"
#include "moses/Syntax/PHyperedge.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SHyperedgeBundle.h"
#include "moses/Syntax/SHyperedgeBundleScorer.h"

#include "PHyperedgeToSHyperedgeBundle.h"
#include "PVertexToStackMap.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

class RuleMatcherCallback
{
private:
  typedef BoundedPriorityContainer<SHyperedgeBundle> Container;

public:
  RuleMatcherCallback(const PVertexToStackMap &stackMap, std::size_t ruleLimit)
    : m_stackMap(stackMap)
    , m_container(ruleLimit) {}

  void operator()(const PHyperedge &hyperedge) {
    PHyperedgeToSHyperedgeBundle(hyperedge, m_stackMap, m_tmpBundle);
    float score = SHyperedgeBundleScorer::Score(m_tmpBundle);
    m_container.SwapIn(m_tmpBundle, score);
  }

  void ClearContainer() {
    m_container.LazyClear();
  }

  const Container &GetContainer() {
    return m_container;
  }

private:
  const PVertexToStackMap &m_stackMap;
  SHyperedgeBundle m_tmpBundle;
  BoundedPriorityContainer<SHyperedgeBundle> m_container;
};

}  // F2S
}  // Syntax
}  // Moses

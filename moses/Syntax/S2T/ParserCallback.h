#pragma once

#include "moses/Syntax/BoundedPriorityContainer.h"
#include "moses/Syntax/PHyperedge.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SHyperedgeBundle.h"
#include "moses/Syntax/SHyperedgeBundleScorer.h"

#include "PHyperedgeToSHyperedgeBundle.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

class StandardParserCallback
{
private:
  typedef BoundedPriorityContainer<SHyperedgeBundle> Container;

public:
  StandardParserCallback(const SChart &schart, std::size_t ruleLimit)
    : m_schart(schart)
    , m_container(ruleLimit) {}

  void operator()(const PHyperedge &hyperedge) {
    PHyperedgeToSHyperedgeBundle(hyperedge, m_schart, m_tmpBundle);
    float score = SHyperedgeBundleScorer::Score(m_tmpBundle);
    m_container.SwapIn(m_tmpBundle, score);
  }

  void InitForRange(const WordsRange &range) {
    m_container.LazyClear();
  }

  const Container &GetContainer() {
    return m_container;
  }

private:
  const SChart &m_schart;
  SHyperedgeBundle m_tmpBundle;
  BoundedPriorityContainer<SHyperedgeBundle> m_container;
};

class EagerParserCallback
{
private:
  typedef BoundedPriorityContainer<SHyperedgeBundle> Container;

public:
  EagerParserCallback(const SChart &schart, std::size_t ruleLimit)
    : m_schart(schart)
    , m_containers(schart.GetWidth(), Container(ruleLimit))
    , m_prevStart(std::numeric_limits<std::size_t>::max()) {}

  void operator()(const PHyperedge &hyperedge, std::size_t end) {
    PHyperedgeToSHyperedgeBundle(hyperedge, m_schart, m_tmpBundle);
    float score = SHyperedgeBundleScorer::Score(m_tmpBundle);
    m_containers[end].SwapIn(m_tmpBundle, score);
  }

  void InitForRange(const WordsRange &range) {
    const std::size_t start = range.GetStartPos();
    m_end = range.GetEndPos();
    if (start != m_prevStart) {
      for (std::vector<Container>::iterator p = m_containers.begin();
           p != m_containers.end(); ++p) {
        p->LazyClear();
      }
      m_prevStart = start;
    }
  }

  const Container &GetContainer() {
    return m_containers[m_end];
  }

private:
  const SChart &m_schart;
  SHyperedgeBundle m_tmpBundle;
  std::vector<Container> m_containers;
  std::size_t m_end;
  std::size_t m_prevStart;
};

}  // S2T
}  // Syntax
}  // Moses

#include "TopologicalSorter.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

void TopologicalSorter::Sort(const Forest &forest,
                             std::vector<const Forest::Vertex *> &permutation)
{
  permutation.clear();
  BuildPredSets(forest);
  m_visited.clear();
  for (std::vector<Forest::Vertex *>::const_iterator
       p = forest.vertices.begin(); p != forest.vertices.end(); ++p) {
    if (m_visited.find(*p) == m_visited.end()) {
      Visit(**p, permutation);
    }
  }
}

void TopologicalSorter::BuildPredSets(const Forest &forest)
{
  m_predSets.clear();
  for (std::vector<Forest::Vertex *>::const_iterator
       p = forest.vertices.begin(); p != forest.vertices.end(); ++p) {
    const Forest::Vertex *head = *p;
    for (std::vector<Forest::Hyperedge *>::const_iterator
         q = head->incoming.begin(); q != head->incoming.end(); ++q) {
      for (std::vector<Forest::Vertex *>::const_iterator
           r = (*q)->tail.begin(); r != (*q)->tail.end(); ++r) {
        m_predSets[head].insert(*r);
      }
    }
  }
}

void TopologicalSorter::Visit(const Forest::Vertex &v,
                              std::vector<const Forest::Vertex *> &permutation)
{
  m_visited.insert(&v);
  const VertexSet &predSet = m_predSets[&v];
  for (VertexSet::const_iterator p = predSet.begin(); p != predSet.end(); ++p) {
    if (m_visited.find(*p) == m_visited.end()) {
      Visit(**p, permutation);
    }
  }
  permutation.push_back(&v);
}

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses

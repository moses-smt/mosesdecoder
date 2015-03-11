#include "InputTreeToForest.h"

#include <boost/unordered_map.hpp>

namespace Moses
{
namespace Syntax
{
namespace T2S
{

const F2S::Forest::Vertex *InputTreeToForest(const InputTree &tree,
    F2S::Forest &forest)
{
  forest.Clear();

  // Map from tree vertices to forest vertices.
  boost::unordered_map<const InputTree::Node*, F2S::Forest::Vertex*> vertexMap;

  // Create forest vertices (but not hyperedges) and fill in vertexMap.
  for (std::vector<InputTree::Node>::const_iterator p = tree.nodes.begin();
       p != tree.nodes.end(); ++p) {
    F2S::Forest::Vertex *v = new F2S::Forest::Vertex(p->pvertex);
    forest.vertices.push_back(v);
    vertexMap[&*p] = v;
  }

  // Create the forest hyperedges.
  for (std::vector<InputTree::Node>::const_iterator p = tree.nodes.begin();
       p != tree.nodes.end(); ++p) {
    const InputTree::Node &treeVertex = *p;
    const std::vector<InputTree::Node*> &treeChildren = treeVertex.children;
    if (treeChildren.empty()) {
      continue;
    }
    F2S::Forest::Hyperedge *e = new F2S::Forest::Hyperedge();
    e->head = vertexMap[&treeVertex];
    e->tail.reserve(treeChildren.size());
    for (std::vector<InputTree::Node*>::const_iterator q = treeChildren.begin();
         q != treeChildren.end(); ++q) {
      e->tail.push_back(vertexMap[*q]);
    }
    e->head->incoming.push_back(e);
  }

  // Return a pointer to the forest's root vertex.
  return forest.vertices.back();
}

}  // T2S
}  // Syntax
}  // Moses

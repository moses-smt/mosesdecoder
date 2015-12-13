#pragma once

#include <vector>

#include "moses/Syntax/PVertex.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

struct InputTree {
public:
  struct Node {
    Node(const PVertex &v, const std::vector<Node*> &c)
      : pvertex(v)
      , children(c) {}

    Node(const PVertex &v) : pvertex(v) {}

    PVertex pvertex;
    std::vector<Node*> children;
  };

  // All tree nodes in post-order.
  std::vector<Node> nodes;

  // Tree nodes arranged by starting position (i.e. the vector nodes[i]
  // contains the subset of tree nodes with span [i,j] (for any j).)
  std::vector<std::vector<Node*> > nodesAtPos;
};

}  // T2S
}  // Syntax
}  // Moses

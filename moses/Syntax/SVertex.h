#pragma once

#include <vector>

namespace Moses
{

class FFState;

namespace Syntax
{

struct PVertex;
struct SHyperedge;

// A vertex in the search hypergraph.
//
// Important: a SVertex owns its incoming SHyperedge objects and its FFState
// objects and will delete them on destruction.
struct SVertex {
  ~SVertex();

  SHyperedge *best;
  std::vector<SHyperedge*> recombined;
  const PVertex *pvertex;
  std::vector<FFState*> state;
};

}  // Syntax
}  // Moses

#pragma once

#include <vector>
#include <stddef.h>

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
  std::vector<FFState*> states;

  // for unordered_set in stack
  size_t hash() const;
  bool operator==(const SVertex& other) const;

};

}  // Syntax
}  // Moses

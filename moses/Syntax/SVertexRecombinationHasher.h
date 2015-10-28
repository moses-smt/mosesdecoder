#pragma once

#include "moses/FF/FFState.h"

#include "SVertex.h"

namespace Moses
{
namespace Syntax
{

class SVertexRecombinationHasher
{
public:
  std::size_t operator()(const SVertex *v) const {
    std::size_t seed = 0;
    for (std::vector<FFState*>::const_iterator p = v->states.begin();
         p != v->states.end(); ++p) {
      boost::hash_combine(seed, (*p)->hash());
    }
    return seed;
  }
};

}  // Syntax
}  // Moses

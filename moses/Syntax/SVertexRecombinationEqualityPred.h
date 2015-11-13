#pragma once

#include "moses/FF/FFState.h"

#include "SVertex.h"

namespace Moses
{
namespace Syntax
{

class SVertexRecombinationEqualityPred
{
public:
  bool operator()(const SVertex *v1, const SVertex *v2) const {
    assert(v1->states.size() == v2->states.size());
    for (std::size_t i = 0; i < v1->states.size(); ++i) {
      if (*(v1->states[i]) != *(v2->states[i])) {
        return false;
      }
    }
    return true;
  }
};

}  // Syntax
}  // Moses

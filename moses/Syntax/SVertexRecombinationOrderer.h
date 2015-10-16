#pragma once

#include "moses/FF/FFState.h"

#include "SVertex.h"

namespace Moses
{
namespace Syntax
{


class SVertexRecombinationUnordered
{
public:
  size_t operator()(const SVertex* hypo) const {
    return hypo->hash();
  }

  bool operator()(const SVertex* hypoA, const SVertex* hypoB) const {
    return (*hypoA) == (*hypoB);
  }

};

}  // Syntax
}  // Moses

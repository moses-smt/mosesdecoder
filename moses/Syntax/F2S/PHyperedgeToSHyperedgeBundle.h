#pragma once

#include "moses/Syntax/PHyperedge.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SHyperedgeBundle.h"

#include "PVertexToStackMap.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// Given a PHyperedge object and SStackSet produces a SHyperedgeBundle object.
inline void PHyperedgeToSHyperedgeBundle(const PHyperedge &hyperedge,
    const PVertexToStackMap &stackMap,
    SHyperedgeBundle &bundle)
{
  bundle.inputWeight = hyperedge.label.inputWeight;
  bundle.translations = hyperedge.label.translations;
  bundle.stacks.clear();
  for (std::vector<PVertex*>::const_iterator p = hyperedge.tail.begin();
       p != hyperedge.tail.end(); ++p) {
    PVertexToStackMap::const_iterator q = stackMap.find(*p);
    const SVertexStack &stack = q->second;
    bundle.stacks.push_back(&stack);
  }
}

}  // F2S
}  // Syntax
}  // Moses

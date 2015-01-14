#pragma once

#include "moses/Syntax/PHyperedge.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SHyperedgeBundle.h"

#include "SChart.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

// Given a PHyperedge object and SChart produces a SHyperedgeBundle object.
inline void PHyperedgeToSHyperedgeBundle(const PHyperedge &hyperedge,
    const SChart &schart,
    SHyperedgeBundle &bundle)
{
  bundle.translations = hyperedge.translations;
  bundle.stacks.clear();
  for (std::vector<PVertex*>::const_iterator p = hyperedge.tail.begin();
       p != hyperedge.tail.end(); ++p) {
    const PVertex *v = *p;
    std::size_t spanStart = v->span.GetStartPos();
    std::size_t spanEnd = v->span.GetEndPos();
    const Word &symbol = v->symbol;
    const SChart::Cell &cell = schart.GetCell(spanStart, spanEnd);
    const SVertexStack *stack = 0;
    if (symbol.IsNonTerminal()) {
      stack = cell.nonTerminalStacks.Find(symbol);
    } else {
      const SChart::Cell::TMap::const_iterator q =
        cell.terminalStacks.find(symbol);
      assert(q != cell.terminalStacks.end());
      stack = &(q->second);
    }
    bundle.stacks.push_back(stack);
  }
}

}  // S2T
}  // Syntax
}  // Moses

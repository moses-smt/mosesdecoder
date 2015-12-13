#pragma once

#include <vector>

#include "moses/Syntax/S2T/PChart.h"

#include "PatternApplicationTrie.h"
#include "SymbolRange.h"
#include "TailLattice.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

class TailLatticeBuilder
{
public:
  TailLatticeBuilder(PChart &chart) : m_chart(chart) {}

  // Given a key from a PatternApplicationTrie and the valid ranges of its
  // symbols, construct a TailLattice.
  void Build(const std::vector<const PatternApplicationTrie *> &,
             const std::vector<SymbolRange> &,
             TailLattice &, std::vector<std::vector<bool> > &);

private:
  // Auxiliary function used by Build.  Enlarges a TailLattice, if necessary,
  // and clears the innermost vectors.
  void ExtendAndClear(const std::vector<const PatternApplicationTrie *> &,
                      const std::vector<SymbolRange> &,
                      TailLattice &, std::vector<std::vector<bool> > &);

  PChart &m_chart;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

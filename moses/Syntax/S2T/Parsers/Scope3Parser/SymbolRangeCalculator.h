#pragma once

#include <vector>

#include "PatternApplicationTrie.h"
#include "SymbolRange.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

class SymbolRangeCalculator
{
public:
  void Calc(const PatternApplicationKey &, int, int,
            std::vector<SymbolRange> &);

private:
  // Provides contextual information used in determining a symbol's range.
  struct AuxSymbolInfo {
    int distanceToNextTerminal;
    int distanceToPrevTerminal;
  };

  void FillInTerminalRanges(const PatternApplicationKey &,
                            std::vector<SymbolRange> &);

  void FillInAuxSymbolInfo(const std::vector<SymbolRange> &);

  void FillInGapRanges(const PatternApplicationKey &, int, int,
                       std::vector<SymbolRange> &);

  std::vector<AuxSymbolInfo> m_auxSymbolInfo;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

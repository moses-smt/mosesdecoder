#pragma once

#include <vector>

#include <boost/unordered_map.hpp>

#include "moses/Syntax/NonTerminalMap.h"
#include "moses/Syntax/SVertexStack.h"
#include "moses/Syntax/SymbolEqualityPred.h"
#include "moses/Syntax/SymbolHasher.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

class SChart
{
public:
  struct Cell {
    typedef boost::unordered_map<Word, SVertexStack, SymbolHasher,
            SymbolEqualityPred> TMap;
    typedef NonTerminalMap<SVertexStack> NMap;
    TMap terminalStacks;
    NMap nonTerminalStacks;
  };

  SChart(std::size_t width);

  std::size_t GetWidth() const {
    return m_cells.size();
  }

  const Cell &GetCell(std::size_t start, std::size_t end) const {
    return m_cells[start][end];
  }

  Cell &GetCell(std::size_t start, std::size_t end) {
    return m_cells[start][end];
  }

private:
  std::vector<std::vector<Cell> > m_cells;
};

}  // S2T
}  // Syntax
}  // Moses

#pragma once

#include <vector>

#include <boost/unordered_map.hpp>

#include "moses/Syntax/NonTerminalMap.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SymbolEqualityPred.h"
#include "moses/Syntax/SymbolHasher.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

class PChart
{
public:
  struct Cell {
    typedef boost::unordered_map<Word, PVertex, SymbolHasher,
            SymbolEqualityPred> TMap;
    typedef NonTerminalMap<PVertex> NMap;
    // Collection of terminal vertices (keyed by terminal symbol).
    TMap terminalVertices;
    // Collection of non-terminal vertices (keyed by non-terminal symbol).
    NMap nonTerminalVertices;
  };

  struct CompressedItem {
    std::size_t end;
    const PVertex *vertex;
  };

  typedef std::vector<std::vector<CompressedItem> > CompressedMatrix;

  PChart(std::size_t width, bool maintainCompressedChart);

  ~PChart();

  std::size_t GetWidth() const {
    return m_cells.size();
  }

  const Cell &GetCell(std::size_t start, std::size_t end) const {
    return m_cells[start][end];
  }

  // Insert the given PVertex and return a reference to the inserted object.
  PVertex &AddVertex(const PVertex &v) {
    const std::size_t start = v.span.GetStartPos();
    const std::size_t end = v.span.GetEndPos();
    Cell &cell = m_cells[start][end];
    // If v is a terminal vertex add it to the cell's terminalVertices map.
    if (!v.symbol.IsNonTerminal()) {
      Cell::TMap::value_type x(v.symbol, v);
      std::pair<Cell::TMap::iterator, bool> ret =
        cell.terminalVertices.insert(x);
      return ret.first->second;
    }
    // If v is a non-terminal vertex add it to the cell's nonTerminalVertices
    // map and update the compressed chart (if enabled).
    std::pair<Cell::NMap::Iterator, bool> result =
      cell.nonTerminalVertices.Insert(v.symbol, v);
    if (result.second && m_compressedChart) {
      CompressedItem item;
      item.end = end;
      item.vertex = &(result.first->second);
      (*m_compressedChart)[start][v.symbol[0]->GetId()].push_back(item);
    }
    return result.first->second;
  }

  const CompressedMatrix &GetCompressedMatrix(std::size_t start) const {
    return (*m_compressedChart)[start];
  }

private:
  typedef std::vector<CompressedMatrix> CompressedChart;

  std::vector<std::vector<Cell> > m_cells;
  CompressedChart *m_compressedChart;
};

}  // S2T
}  // Syntax
}  // Moses

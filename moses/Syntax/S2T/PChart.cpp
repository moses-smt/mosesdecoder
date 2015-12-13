#include "PChart.h"

#include "moses/FactorCollection.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

PChart::PChart(std::size_t width, bool maintainCompressedChart)
{
  m_cells.resize(width);
  for (std::size_t i = 0; i < width; ++i) {
    m_cells[i].resize(width);
  }
  if (maintainCompressedChart) {
    m_compressedChart = new CompressedChart(width);
    for (CompressedChart::iterator p = m_compressedChart->begin();
         p != m_compressedChart->end(); ++p) {
      p->resize(FactorCollection::Instance().GetNumNonTerminals());
    }
  }
}

PChart::~PChart()
{
  delete m_compressedChart;
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

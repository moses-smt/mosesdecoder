#include "SChart.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

SChart::SChart(std::size_t width)
{
  m_cells.resize(width);
  for (std::size_t i = 0; i < width; ++i) {
    m_cells[i].resize(width);
  }
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses

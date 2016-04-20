#include "ActiveChart.h"
#include "InputPath.h"

namespace Moses2
{
namespace SCFG
{

ActiveChartEntry::ActiveChartEntry(const SCFG::InputPath *subPhrasePath, bool isNT)
{
  if (subPhrasePath) {
    symbolBinds.Add(subPhrasePath->range, isNT);
  }
}


}
}


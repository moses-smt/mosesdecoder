#include <boost/foreach.hpp>
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

std::ostream& operator<<(std::ostream &out, const SymbolBind &obj)
{
  BOOST_FOREACH(const SymbolBind::Element &ele, obj.coll) {
    out << "("<< *ele.first << " " << ele.second << ") ";
  }

  return out;
}

}
}


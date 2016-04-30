#include <boost/foreach.hpp>
#include "ActiveChart.h"
#include "InputPath.h"

namespace Moses2
{
namespace SCFG
{

ActiveChartEntry::ActiveChartEntry(const SCFG::InputPath *subPhrasePath, const SCFG::Word &word)
{
  if (subPhrasePath) {
    symbolBinds.Add(subPhrasePath->range, word);
  }
}

std::ostream& operator<<(std::ostream &out, const SymbolBind &obj)
{
  BOOST_FOREACH(const SymbolBind::Element &ele, obj.coll) {
    out << "("<< *ele.first << " " << *ele.second << ") ";
  }

  return out;
}

}
}


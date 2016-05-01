#include <boost/foreach.hpp>
#include <boost/functional/hash_fwd.hpp>
#include "ActiveChart.h"
#include "InputPath.h"

namespace Moses2
{
namespace SCFG
{

size_t hash_value(const SymbolBindElement &obj)
{
  size_t ret = (size_t) obj.range;
  boost::hash_combine(ret, obj.word);

  return ret;
}

////////////////////////////////////////////////////////////////////////////
ActiveChartEntry::ActiveChartEntry(
    const SCFG::InputPath &subPhrasePath,
    const SCFG::Word &word,
    const ActiveChartEntry &prevEntry)
:symbolBinds(prevEntry.symbolBinds)
{
  symbolBinds.Add(subPhrasePath.range, word);
}

std::ostream& operator<<(std::ostream &out, const SymbolBind &obj)
{
  BOOST_FOREACH(const SymbolBindElement &ele, obj.coll) {
    out << "("<< *ele.range << *ele.word << ") ";
  }

  return out;
}

}
}


#pragma once
#include <vector>

namespace Moses2
{
namespace SCFG
{
class InputPath;

////////////////////////////////////////////////////////////////////////////
//! The range covered by each symbol in the source
//! Terminals will cover only 1 word, NT can cover multiple words
class SymbolBind
{
public:
  typedef std::pair<Range, bool> Element;
    // range, isNT
  std::vector<Element> coll;

};

////////////////////////////////////////////////////////////////////////////
class ActiveChartEntry
{
public:
  const SCFG::InputPath *subPhrasePath;

  ActiveChartEntry(const SCFG::InputPath *vSubPhrasePath)
  :subPhrasePath(vSubPhrasePath)
  { }

protected:
};

////////////////////////////////////////////////////////////////////////////
class ActiveChart
{
public:
  std::vector<ActiveChartEntry*> entries;
};

}
}


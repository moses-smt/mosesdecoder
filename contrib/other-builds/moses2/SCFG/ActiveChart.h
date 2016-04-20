#pragma once
#include <vector>
#include <iostream>
#include <boost/functional/hash/hash.hpp>
#include "../legacy/Range.h"

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
  friend std::ostream& operator<<(std::ostream &, const SymbolBind &);

public:
  typedef std::pair<const Range*, bool> Element;
    // range, isNT
  std::vector<Element> coll;

  void Add(const Range &range, bool isNT)
  {
    Element ele(&range, isNT);
    coll.push_back(ele);
  }

  bool operator==(const SymbolBind &compare) const
  {
    return coll == compare.coll;
  }

};

inline size_t hash_value(const SymbolBind &obj)
{
  return boost::hash_value(obj.coll);
}

////////////////////////////////////////////////////////////////////////////
class ActiveChartEntry
{
public:
  SymbolBind symbolBinds;

  ActiveChartEntry(const SCFG::InputPath *subPhrasePath, bool isNT);

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


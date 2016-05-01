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
class Word;

////////////////////////////////////////////////////////////////////////////
//! The range covered by each symbol in the source
//! Terminals will cover only 1 word, NT can cover multiple words
class SymbolBindElement
{
public:
  const Range *range;
  const SCFG::Word *word;

  SymbolBindElement(const Range *range, const SCFG::Word *word)
  :range(range)
  ,word(word)
  {}

  bool operator==(const SymbolBindElement &compare) const
  {
    bool ret = range == compare.range
            && word == compare.word;
    return ret;
  }
};

size_t hash_value(const SymbolBindElement &obj);

////////////////////////////////////////////////////////////////////////////
class SymbolBind
{
  friend std::ostream& operator<<(std::ostream &, const SymbolBind &);

public:
  std::vector<SymbolBindElement> coll;

  void Add(const Range &range, const SCFG::Word &word)
  {
    SymbolBindElement ele(&range, &word);
    coll.push_back(ele);
  }

  bool operator==(const SymbolBind &compare) const
  {  return coll == compare.coll; }
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

  ActiveChartEntry()
  {}

  ActiveChartEntry(const SCFG::InputPath &subPhrasePath, const SCFG::Word &word, const ActiveChartEntry &prevEntry);

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


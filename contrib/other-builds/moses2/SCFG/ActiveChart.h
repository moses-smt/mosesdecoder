#pragma once
#include <vector>
#include <iostream>
#include <boost/functional/hash/hash.hpp>
#include "../legacy/Range.h"

namespace Moses2
{
class HypothesisColl;

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
  const Moses2::HypothesisColl *hypos;

  SymbolBindElement() {}
  SymbolBindElement(const Range *range, const SCFG::Word *word, const Moses2::HypothesisColl *hypos);

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
  typedef Vector<SymbolBindElement> Coll;
  Coll coll;
  size_t numNT;

  SymbolBind(MemPool &pool)
  :coll(pool)
  ,numNT(0)
  {}

  SymbolBind(MemPool &pool, const SymbolBind &copy)
  :coll(copy.coll)
  ,numNT(copy.numNT)
  {}

  std::vector<const SymbolBindElement*> GetNTElements() const;

  void Add(const Range &range, const SCFG::Word &word, const Moses2::HypothesisColl *hypos);

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
  ActiveChartEntry(MemPool &pool)
  :symbolBinds(pool)
  {
    //symbolBinds = new (pool.Allocate<SymbolBind>()) SymbolBind(pool);
  }

  ActiveChartEntry(MemPool &pool, const ActiveChartEntry &prevEntry)
  :symbolBinds(pool, prevEntry.GetSymbolBind())
  {
    //symbolBinds = new (pool.Allocate<SymbolBind>()) SymbolBind(pool, *prevEntry.symbolBinds);
  }

  const SymbolBind &GetSymbolBind() const
  { return symbolBinds; }

  SymbolBind &GetSymbolBind()
  { return symbolBinds; }

protected:
  SymbolBind symbolBinds;

};

////////////////////////////////////////////////////////////////////////////
class ActiveChart
{
public:
  ActiveChart(MemPool &pool);
  ~ActiveChart();

  Vector<ActiveChartEntry*> *entries;
};

}
}

